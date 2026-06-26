#include "ScoreEditorTimeline.h"
#include "../../Application.h"
#include "../../ApplicationConfiguration.h"
#include "../../AudioTrackUtils.h"
#include "../../IO.h"
#include "../../Localization.h"
#include "../../Tempo.h"
#include "../../Utilities.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace MikuMikuWorld
{
	ImRect ScoreEditorTimeline::getAudioClipRect(ScoreContext& context, const AudioClip& clip) const
	{
		const float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const int startTick = accumulateTicks(clip.timelineStartMs / 1000.0f, TICKS_PER_BEAT,
		                                      context.score.tempoChanges);
		const int endTick = accumulateTicks((clip.timelineStartMs + durationMs) / 1000.0f,
		                                    TICKS_PER_BEAT, context.score.tempoChanges);
		const float y1 = position.y - tickToPosition(startTick) + visualOffset;
		const float y2 = position.y - tickToPosition(endTick) + visualOffset;
		const float minX = getTimelineStartX(context) + (8.0f * dpiScale);
		const float maxX = getTimelineEndX(context) - (8.0f * dpiScale);
		const float availableWidth = std::max(1.0f, maxX - minX);
		const float clipWidth = std::min(260.0f * dpiScale, availableWidth);
		const float centerX = midpoint(getTimelineStartX(context), getTimelineEndX(context));
		const float x1 = std::clamp(centerX - (clipWidth * 0.5f), minX, maxX - clipWidth);
		const float x2 = x1 + clipWidth;
		return ImRect(ImVec2(x1, std::min(y1, y2)), ImVec2(x2, std::max(y1, y2)));
	}

	float ScoreEditorTimeline::getTimelineMsAtScreenY(const ScoreContext& context,
	                                                  float screenY) const
	{
		const float tickPosition = position.y + visualOffset - screenY;
		const float rawTick =
		    std::max(0.0f, tickPosition / std::max(0.0001f, unitHeight * zoom));
		const int baseTick = static_cast<int>(std::floor(rawTick));
		const float fraction = rawTick - baseTick;
		const float start =
		    accumulateDuration(baseTick, TICKS_PER_BEAT, context.score.tempoChanges);
		const float end =
		    accumulateDuration(baseTick + 1, TICKS_PER_BEAT, context.score.tempoChanges);
		return (start + ((end - start) * fraction)) * 1000.0f;
	}

	float ScoreEditorTimeline::getScreenYFromTimelineMs(const ScoreContext& context,
	                                                    float timelineMs) const
	{
		const int tick = accumulateTicks(timelineMs / 1000.0f, TICKS_PER_BEAT,
		                                 context.score.tempoChanges);
		return position.y - tickToPosition(tick) + visualOffset;
	}

	float ScoreEditorTimeline::snapTimelineMs(const ScoreContext& context, float timelineMs,
	                                          bool roundDown) const
	{
		const int tick = accumulateTicks(timelineMs / 1000.0f, TICKS_PER_BEAT,
		                                 context.score.tempoChanges);
		const int snappedTick = roundDown ? roundTickDown(tick, division) : snapTick(tick, division);
		return static_cast<float>(accumulateDuration(snappedTick, TICKS_PER_BEAT,
		                                             context.score.tempoChanges) *
		                          1000.0);
	}

	float ScoreEditorTimeline::getMouseTimelineMs(const ScoreContext& context, bool snap) const
	{
		const float timelineMs = getTimelineMsAtScreenY(context, ImGui::GetIO().MousePos.y);
		if (!snap)
			return timelineMs;
		return snapTimelineMs(context, timelineMs, snapMode != SnapMode::Relative);
	}

	void ScoreEditorTimeline::drawAudioClipWaveform(ScoreContext& context, const AudioClip& clip,
	                                                const ImRect& rect, ImU32 leftColor,
	                                                ImU32 rightColor)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList || context.sourceWaveformL.isEmpty())
			return;

		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const float clipPadding = 8.0f * ImGui::GetMainViewport()->DpiScale;
		const float centerX = midpoint(rect.Min.x, rect.Max.x);
		const float maxBarValue = std::max(1.0f, (rect.GetWidth() * 0.5f) - clipPadding);
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);

		drawList->PushClipRect(rect.Min, rect.Max, true);
		const float screenYStart = std::max(rect.Min.y, position.y);
		const float screenYEnd = std::min(rect.Max.y, position.y + size.y);
		const int timelineYStart =
		    static_cast<int>(std::floor(position.y + visualOffset - screenYEnd));
		const int timelineYEnd =
		    static_cast<int>(std::ceil(position.y + visualOffset - screenYStart));
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		for (size_t channelIndex = 0; channelIndex < 2; ++channelIndex)
		{
			const bool rightChannel = channelIndex == 1;
			const Audio::WaveformMipChain& waveform =
			    rightChannel ? context.sourceWaveformR : context.sourceWaveformL;
			if (waveform.isEmpty())
				continue;

			const ImU32 color = rightChannel ? rightColor : leftColor;
			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);
			for (int timelineY = timelineYStart; timelineY <= timelineYEnd; ++timelineY)
			{
				const float screenY = std::floor(position.y + visualOffset - timelineY);
				if (screenY < screenYStart || screenY > screenYEnd)
					continue;

				const int tick = positionToTick(timelineY);
				const float timelineMs =
				    static_cast<float>(accumulateDuration(tick, TICKS_PER_BEAT,
				                                          context.score.tempoChanges) *
				                       1000.0);
				const float sourceMs = clip.sourceStartMs + (timelineMs - clip.timelineStartMs);
				if (sourceMs < clip.sourceStartMs || sourceMs > sourceEndMs)
					continue;

				const float clipTimeMs = sourceMs - clip.sourceStartMs;
				float envelope = clip.gain;
				if (clip.fadeInMs > 0.0f)
					envelope *= std::clamp(clipTimeMs / clip.fadeInMs, 0.0f, 1.0f);
				if (clip.fadeOutMs > 0.0f)
					envelope *= std::clamp((durationMs - clipTimeMs) / clip.fadeOutMs, 0.0f, 1.0f);

				const double sourceSeconds = sourceMs / 1000.0;
				const float amplitude = std::max(
				    waveform.getAmplitudeAt(mip, sourceSeconds, secondsPerPixel), 0.0f);
				const float barValue = amplitude * envelope * maxBarValue;
				const float halfBar = std::max(0.75f, barValue);
				const ImVec2 p1(centerX, screenY);
				const ImVec2 p2(centerX + (halfBar * (rightChannel ? 1.0f : -1.0f)),
				                screenY + 0.75f);
				drawList->AddRectFilled(p1, p2, color);
			}
		}
		drawList->PopClipRect();
	}

	bool ScoreEditorTimeline::hasAudioRangeSelection(id_t clipId) const
	{
		if (audioRangeClip != clipId)
			return false;
		return std::abs(audioRangeEndMs - audioRangeStartMs) >= 1.0f;
	}

	bool ScoreEditorTimeline::splitAudioClipAt(ScoreContext& context, id_t clipId,
	                                           float timelineMs)
	{
		for (AudioClip& clip : context.score.audioTrack.clips)
		{
			if (clip.ID != clipId)
				continue;

			const float sourceEndMs =
			    clip.sourceEndMs >= 0.0f
			        ? clip.sourceEndMs
			        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
			const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
			const float clipStartMs = clip.timelineStartMs;
			const float clipEndMs = clip.timelineStartMs + durationMs;
			if (timelineMs <= clipStartMs + 1.0f || timelineMs >= clipEndMs - 1.0f)
				return false;

			const Score prev = context.score;
			const float splitSourceMs = clip.sourceStartMs + (timelineMs - clipStartMs);

			AudioClip rightClip = clip;
			rightClip.ID = getNextAudioClipID();
			rightClip.sourceStartMs = splitSourceMs;
			rightClip.timelineStartMs = timelineMs;
			rightClip.fadeInMs = 0.0f;

			clip.sourceEndMs = splitSourceMs;
			clip.fadeOutMs = 0.0f;

			context.score.audioTrack.clips.push_back(rightClip);
			context.score.audioTrack.explicitEditorTrack = true;
			context.selectedAudioClip = rightClip.ID;
			audioRangeClip = static_cast<id_t>(-1);
			AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
			context.pushHistory("Split Audio Clip", prev, context.score);
			Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
			if (!refreshResult.isOk())
				IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
				               IO::MessageBoxIcon::Warning);
			return true;
		}

		return false;
	}

	bool ScoreEditorTimeline::splitAudioClipRange(ScoreContext& context, id_t clipId, float startMs,
	                                              float endMs)
	{
		if (endMs < startMs)
			std::swap(startMs, endMs);
		if (endMs - startMs < 1.0f)
			return false;

		auto& clips = context.score.audioTrack.clips;
		auto it = std::find_if(clips.begin(), clips.end(),
		                       [clipId](const AudioClip& clip) { return clip.ID == clipId; });
		if (it == clips.end())
			return false;

		const AudioClip clip = *it;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const float clipStartMs = clip.timelineStartMs;
		const float clipEndMs = clip.timelineStartMs + durationMs;
		const float splitStartMs = std::clamp(startMs, clipStartMs, clipEndMs);
		const float splitEndMs = std::clamp(endMs, clipStartMs, clipEndMs);
		const bool hasStartSplit = splitStartMs > clipStartMs + 1.0f;
		const bool hasEndSplit = splitEndMs < clipEndMs - 1.0f;
		if (!hasStartSplit && !hasEndSplit)
			return false;

		const Score prev = context.score;
		std::vector<AudioClip> replacementClips;

		auto makeSegment = [&](float segmentStartMs, float segmentEndMs, bool keepOriginalId)
		{
			AudioClip segment = clip;
			segment.ID = keepOriginalId ? clip.ID : getNextAudioClipID();
			segment.timelineStartMs = segmentStartMs;
			segment.sourceStartMs = clip.sourceStartMs + (segmentStartMs - clipStartMs);
			segment.sourceEndMs = clip.sourceStartMs + (segmentEndMs - clipStartMs);
			if (segmentStartMs > clipStartMs + 1.0f)
				segment.fadeInMs = 0.0f;
			if (segmentEndMs < clipEndMs - 1.0f)
				segment.fadeOutMs = 0.0f;
			replacementClips.push_back(segment);
		};

		const float middleStartMs = hasStartSplit ? splitStartMs : clipStartMs;
		const float middleEndMs = hasEndSplit ? splitEndMs : clipEndMs;
		if (hasStartSplit)
			makeSegment(clipStartMs, splitStartMs, true);
		makeSegment(middleStartMs, middleEndMs, !hasStartSplit);
		if (hasEndSplit)
			makeSegment(splitEndMs, clipEndMs, false);

		const size_t index = static_cast<size_t>(std::distance(clips.begin(), it));
		clips.erase(clips.begin() + index);
		clips.insert(clips.begin() + index, replacementClips.begin(), replacementClips.end());
		context.score.audioTrack.explicitEditorTrack = true;
		context.selectedAudioClip = replacementClips.size() > 1
		                                ? replacementClips[hasStartSplit ? 1 : 0].ID
		                                : replacementClips.front().ID;
		audioRangeClip = context.selectedAudioClip;
		audioRangeStartMs = middleStartMs;
		audioRangeEndMs = middleEndMs;
		AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
		context.pushHistory("Split Audio Range", prev, context.score);
		Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
		if (!refreshResult.isOk())
			IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning);
		return true;
	}

	bool ScoreEditorTimeline::cutAudioClipRange(ScoreContext& context, id_t clipId, float startMs,
	                                            float endMs)
	{
		if (endMs < startMs)
			std::swap(startMs, endMs);
		if (endMs - startMs < 1.0f)
			return false;

		auto& clips = context.score.audioTrack.clips;
		auto it = std::find_if(clips.begin(), clips.end(),
		                       [clipId](const AudioClip& clip) { return clip.ID == clipId; });
		if (it == clips.end())
			return false;

		const AudioClip clip = *it;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const float clipStartMs = clip.timelineStartMs;
		const float clipEndMs = clip.timelineStartMs + durationMs;
		const float cutStartMs = std::clamp(startMs, clipStartMs, clipEndMs);
		const float cutEndMs = std::clamp(endMs, clipStartMs, clipEndMs);
		if (cutEndMs - cutStartMs < 1.0f)
			return false;

		const Score prev = context.score;
		std::vector<AudioClip> replacementClips;
		const float leftDurationMs = cutStartMs - clipStartMs;
		const float rightOffsetMs = cutEndMs - clipStartMs;

		if (leftDurationMs > 1.0f)
		{
			AudioClip leftClip = clip;
			leftClip.sourceEndMs = clip.sourceStartMs + leftDurationMs;
			if (cutEndMs < clipEndMs - 1.0f)
				leftClip.fadeOutMs = 0.0f;
			replacementClips.push_back(leftClip);
		}

		if (cutEndMs < clipEndMs - 1.0f)
		{
			AudioClip rightClip = clip;
			rightClip.ID = getNextAudioClipID();
			rightClip.sourceStartMs = clip.sourceStartMs + rightOffsetMs;
			rightClip.sourceEndMs = sourceEndMs;
			rightClip.timelineStartMs = cutEndMs;
			if (cutStartMs > clipStartMs + 1.0f)
				rightClip.fadeInMs = 0.0f;
			replacementClips.push_back(rightClip);
		}

		const size_t index = static_cast<size_t>(std::distance(clips.begin(), it));
		clips.erase(clips.begin() + index);
		clips.insert(clips.begin() + index, replacementClips.begin(), replacementClips.end());
		context.score.audioTrack.explicitEditorTrack = true;
		context.selectedAudioClip =
		    replacementClips.empty() ? static_cast<id_t>(-1) : replacementClips.back().ID;
		audioRangeClip = static_cast<id_t>(-1);
		AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
		context.pushHistory("Cut Audio Range", prev, context.score);
		Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
		if (!refreshResult.isOk())
			IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning);
		return true;
	}

	void ScoreEditorTimeline::drawAudioTrack(ScoreContext& context)
	{
		const bool audioEditing = context.isAudioEditing();
		if (!audioEditing && !config.drawWaveform)
			return;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		for (const AudioClip& clip : context.score.audioTrack.clips)
		{
			if (!clip.visible)
				continue;

			const ImRect rect = getAudioClipRect(context, clip);
			if (rect.Max.y < position.y || rect.Min.y > position.y + size.y)
				continue;

			const bool selected = audioEditing && context.selectedAudioClip == clip.ID;
			const ImU32 fillColor =
			    audioEditing ? (selected ? IM_COL32(48, 78, 96, 92)
			                             : IM_COL32(48, 78, 96, 48))
			                 : IM_COL32(48, 78, 96, 18);
			const ImU32 borderColor =
			    selected ? IM_COL32(165, 220, 255, 230) : IM_COL32(135, 180, 210, 130);
			drawList->AddRectFilled(rect.Min, rect.Max, fillColor, 4.0f);
			drawAudioClipWaveform(context, clip, rect,
			                      audioEditing ? IM_COL32(185, 218, 238, 150)
			                                   : IM_COL32(185, 218, 238, 58),
			                      audioEditing ? IM_COL32(155, 195, 220, 130)
			                                   : IM_COL32(155, 195, 220, 45));

			if (audioEditing && hasAudioRangeSelection(clip.ID))
			{
				const float rangeStartMs = std::min(audioRangeStartMs, audioRangeEndMs);
				const float rangeEndMs = std::max(audioRangeStartMs, audioRangeEndMs);
				const float y1 = getScreenYFromTimelineMs(context, rangeStartMs);
				const float y2 = getScreenYFromTimelineMs(context, rangeEndMs);
				const ImRect rangeRect(ImVec2(rect.Min.x, std::min(y1, y2)),
				                       ImVec2(rect.Max.x, std::max(y1, y2)));
				const ImRect clippedRange(ImVec2(rangeRect.Min.x, std::max(rangeRect.Min.y, rect.Min.y)),
				                          ImVec2(rangeRect.Max.x, std::min(rangeRect.Max.y, rect.Max.y)));
				if (clippedRange.Max.y > clippedRange.Min.y)
				{
					drawList->AddRectFilled(clippedRange.Min, clippedRange.Max,
					                        IM_COL32(255, 255, 255, 38), 2.0f);
					drawList->AddRect(clippedRange.Min, clippedRange.Max,
					                  IM_COL32(255, 255, 255, 130), 2.0f);
				}
			}

			drawList->AddRect(rect.Min, rect.Max, borderColor, 4.0f, 0,
			                  audioEditing ? 2.0f : 1.0f);

			if (audioEditing)
			{
				const float dpiScale = ImGui::GetMainViewport()->DpiScale;
				const float grabWidth = 14.0f * dpiScale;
				const float knobRadius = 4.0f * dpiScale;
				const float sourceEndMs =
				    clip.sourceEndMs >= 0.0f
				        ? clip.sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
				const ImU32 handleColor =
				    context.score.audioTrack.locked || clip.locked ? IM_COL32(180, 180, 180, 120)
				                                                   : IM_COL32(225, 240, 255, 180);
				drawList->AddRectFilled(rect.Min, ImVec2(rect.Min.x + grabWidth, rect.Max.y),
				                        selected ? IM_COL32(130, 190, 230, 92)
				                                 : IM_COL32(130, 190, 230, 55),
				                        4.0f, ImDrawFlags_RoundCornersLeft);

				const float fadeInY =
				    std::clamp(getScreenYFromTimelineMs(context, clip.timelineStartMs + clip.fadeInMs),
				               rect.Min.y, rect.Max.y);
				const float fadeOutY = std::clamp(
				    getScreenYFromTimelineMs(context,
				                             clip.timelineStartMs + durationMs - clip.fadeOutMs),
				    rect.Min.y, rect.Max.y);
				const ImVec2 fadeInHandle(rect.Min.x + (14.0f * dpiScale), fadeInY);
				const ImVec2 fadeOutHandle(rect.Max.x - (14.0f * dpiScale), fadeOutY);
				drawList->AddLine(ImVec2(rect.Min.x, fadeInY), ImVec2(rect.Max.x, fadeInY),
				                  IM_COL32(255, 255, 255, clip.fadeInMs > 0.0f ? 110 : 45),
				                  clip.fadeInMs > 0.0f ? 1.5f : 1.0f);
				drawList->AddLine(ImVec2(rect.Min.x, fadeOutY), ImVec2(rect.Max.x, fadeOutY),
				                  IM_COL32(255, 255, 255, clip.fadeOutMs > 0.0f ? 110 : 45),
				                  clip.fadeOutMs > 0.0f ? 1.5f : 1.0f);
				drawList->AddCircleFilled(fadeInHandle, knobRadius, handleColor);
				drawList->AddCircleFilled(fadeOutHandle, knobRadius, handleColor);

				const std::string clipName =
				    clip.sourceFile.empty() ? context.workingData.musicFilename : clip.sourceFile;
				const std::string label = IO::File::getFilename(clipName);
				if (!label.empty() && rect.GetHeight() > ImGui::GetTextLineHeight() + 8.0f)
				{
					drawList->AddText(ImVec2(rect.Min.x + 6.0f * dpiScale,
					                         rect.Min.y + 6.0f * dpiScale),
					                  IM_COL32(230, 240, 248, 170), label.c_str());
				}
			}
		}
	}

	void ScoreEditorTimeline::updateAudioTrackEditing(ScoreContext& context)
	{
		suppressTimelineContextMenu = false;
		if (!context.isAudioEditing() || context.score.audioTrack.locked)
			return;

		const ImGuiIO& io = ImGui::GetIO();
		const bool useSnap = io.KeyShift;
		const float currentTimelineMs = getMouseTimelineMs(context, useSnap);
		const float playheadTimelineMs = static_cast<float>(
		    accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges) *
		    1000.0);

		auto findClip = [&](id_t id) -> AudioClip*
		{
			for (AudioClip& clip : context.score.audioTrack.clips)
			{
				if (clip.ID == id)
					return &clip;
			}
			return nullptr;
		};

		auto warnRefreshFailure = [](const Result& result)
		{
			if (!result.isOk())
				IO::messageBox(APP_NAME, result.getMessage(), IO::MessageBoxButtons::Ok,
				               IO::MessageBoxIcon::Warning);
		};

		if (ImGui::BeginPopup("audio_clip_context"))
		{
			AudioClip* menuClip = findClip(audioContextMenuClip);
			bool canCut = menuClip && audioContextMenuHasRange;
			bool canSplit = false;
			if (menuClip)
			{
				const float sourceEndMs =
				    menuClip->sourceEndMs >= 0.0f
				        ? menuClip->sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float durationMs = std::max(1.0f, sourceEndMs - menuClip->sourceStartMs);
				if (audioContextMenuHasRange)
				{
					const float rangeStartMs =
					    std::min(audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
					const float rangeEndMs =
					    std::max(audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
					canSplit = rangeStartMs > menuClip->timelineStartMs + 1.0f ||
					           rangeEndMs < menuClip->timelineStartMs + durationMs - 1.0f;
				}
				else
				{
					canSplit = playheadTimelineMs > menuClip->timelineStartMs + 1.0f &&
					           playheadTimelineMs < menuClip->timelineStartMs + durationMs - 1.0f;
				}
			}

			if (ImGui::MenuItem(getString("cut"), nullptr, false, canCut))
			{
				cutAudioClipRange(context, audioContextMenuClip,
				                  audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
				audioContextMenuHasRange = false;
			}
			if (ImGui::MenuItem(getString("audio_clip_split"), nullptr, false, canSplit))
			{
				if (audioContextMenuHasRange)
				{
					splitAudioClipRange(context, audioContextMenuClip,
					                    audioContextMenuRangeStartMs,
					                    audioContextMenuRangeEndMs);
				}
				else
				{
					splitAudioClipAt(context, audioContextMenuClip, playheadTimelineMs);
				}
				audioContextMenuHasRange = false;
			}
			ImGui::EndPopup();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Delete) &&
		    context.selectedAudioClip != static_cast<id_t>(-1))
		{
			if (hasAudioRangeSelection(context.selectedAudioClip))
			{
				cutAudioClipRange(context, context.selectedAudioClip, audioRangeStartMs,
				                  audioRangeEndMs);
				return;
			}

			Score prev = context.score;
			context.score.audioTrack.explicitEditorTrack = true;
			auto& clips = context.score.audioTrack.clips;
			clips.erase(std::remove_if(clips.begin(), clips.end(),
			                           [&](const AudioClip& clip)
			                           { return clip.ID == context.selectedAudioClip; }),
			            clips.end());
			context.selectedAudioClip = static_cast<id_t>(-1);
			audioRangeClip = static_cast<id_t>(-1);
			context.pushHistory("Delete Audio Clip", prev, context.score);
			warnRefreshFailure(AudioTrackUtils::refreshPlaybackAudio(context));
			return;
		}

		if (audioDragMode == AudioDragMode::None)
		{
			for (auto it = context.score.audioTrack.clips.rbegin();
			     it != context.score.audioTrack.clips.rend(); ++it)
			{
				AudioClip& clip = *it;
				if (clip.locked)
					continue;

				const ImRect rect = getAudioClipRect(context, clip);
				if (!rect.Contains(io.MousePos))
					continue;

				const float dpiScale = ImGui::GetMainViewport()->DpiScale;
				const float edgeThreshold = 8.0f * dpiScale;
				const float sourceEndMs =
				    clip.sourceEndMs >= 0.0f
				        ? clip.sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float clipDurationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
				const ImVec2 fadeInHandle(rect.Min.x + (14.0f * dpiScale),
				                          std::clamp(getScreenYFromTimelineMs(
				                                         context, clip.timelineStartMs + clip.fadeInMs),
				                                     rect.Min.y, rect.Max.y));
				const ImVec2 fadeOutHandle(rect.Max.x - (14.0f * dpiScale),
				                           std::clamp(
				                               getScreenYFromTimelineMs(
				                                   context, clip.timelineStartMs + clipDurationMs -
				                                                clip.fadeOutMs),
				                               rect.Min.y, rect.Max.y));
				const ImRect fadeInHandleRect(
				    ImVec2(fadeInHandle.x - 8.0f * dpiScale, fadeInHandle.y - 8.0f * dpiScale),
				    ImVec2(fadeInHandle.x + 8.0f * dpiScale, fadeInHandle.y + 8.0f * dpiScale));
				const ImRect fadeOutHandleRect(
				    ImVec2(fadeOutHandle.x - 8.0f * dpiScale, fadeOutHandle.y - 8.0f * dpiScale),
				    ImVec2(fadeOutHandle.x + 8.0f * dpiScale, fadeOutHandle.y + 8.0f * dpiScale));
				const float grabWidth = 16.0f * dpiScale;
				AudioDragMode hoverMode = AudioDragMode::RangeSelect;
				if (fadeInHandleRect.Contains(io.MousePos) && clipDurationMs > 1.0f)
					hoverMode = AudioDragMode::FadeIn;
				else if (fadeOutHandleRect.Contains(io.MousePos) && clipDurationMs > 1.0f)
					hoverMode = AudioDragMode::FadeOut;
				else if (std::abs(io.MousePos.y - rect.Min.y) <= edgeThreshold)
					hoverMode = AudioDragMode::TrimEnd;
				else if (std::abs(io.MousePos.y - rect.Max.y) <= edgeThreshold)
					hoverMode = AudioDragMode::TrimStart;
				else if (io.MousePos.x <= rect.Min.x + grabWidth)
					hoverMode = AudioDragMode::Move;

				if (hoverMode == AudioDragMode::Move)
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				else if (hoverMode != AudioDragMode::RangeSelect)
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					context.clearSelection();
					context.timelineEditTarget = TimelineEditTarget::Audio;
					context.selectedAudioClip = clip.ID;
					audioContextMenuClip = clip.ID;
					audioContextMenuHasRange = hasAudioRangeSelection(clip.ID);
					audioContextMenuRangeStartMs = audioRangeStartMs;
					audioContextMenuRangeEndMs = audioRangeEndMs;
					suppressTimelineContextMenu = true;
					ImGui::OpenPopup("audio_clip_context");
					break;
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					context.clearSelection();
					context.timelineEditTarget = TimelineEditTarget::Audio;
					context.selectedAudioClip = clip.ID;
					draggingAudioClip = clip.ID;
					audioDragStartClip = clip;
					audioDragStartScore = context.score;
					audioDragStartTimelineMs = currentTimelineMs;
					audioDragMode = hoverMode;
					if (audioDragMode == AudioDragMode::RangeSelect)
					{
						audioRangeClip = clip.ID;
						audioRangeStartMs = currentTimelineMs;
						audioRangeEndMs = currentTimelineMs;
					}
					else
					{
						audioRangeClip = static_cast<id_t>(-1);
					}
					break;
				}
			}
		}

		if (audioDragMode != AudioDragMode::None)
		{
			AudioClip* clip = findClip(draggingAudioClip);
			if (!clip)
			{
				audioDragMode = AudioDragMode::None;
				return;
			}

			const float deltaMs = currentTimelineMs - audioDragStartTimelineMs;
			const float sourceLengthMs =
			    static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
			const float startSourceEndMs =
			    audioDragStartClip.sourceEndMs >= 0.0f ? audioDragStartClip.sourceEndMs
			                                           : sourceLengthMs;
			switch (audioDragMode)
			{
			case AudioDragMode::Move:
			{
				clip->timelineStartMs = audioDragStartClip.timelineStartMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
					clip->timelineStartMs =
					    snapTimelineMs(context, clip->timelineStartMs, true);

				const float movingDurationMs = std::max(1.0f, startSourceEndMs -
				                                             audioDragStartClip.sourceStartMs);
				const float edgeSnapThresholdMs = std::max(
				    20.0f,
				    std::abs(getTimelineMsAtScreenY(context, io.MousePos.y + 10.0f) -
				             getTimelineMsAtScreenY(context, io.MousePos.y)));
				const float candidateStartMs = clip->timelineStartMs;
				const float candidateEndMs = candidateStartMs + movingDurationMs;
				float bestStartMs = candidateStartMs;
				float bestDistanceMs = edgeSnapThresholdMs;
				auto trySnapEdge = [&](float movingEdgeMs, float targetEdgeMs, float edgeOffsetMs)
				{
					const float distance = std::abs(movingEdgeMs - targetEdgeMs);
					if (distance <= bestDistanceMs)
					{
						bestDistanceMs = distance;
						bestStartMs = targetEdgeMs - edgeOffsetMs;
					}
				};

				for (const AudioClip& otherClip : context.score.audioTrack.clips)
				{
					if (otherClip.ID == clip->ID)
						continue;

					const float otherSourceEndMs =
					    otherClip.sourceEndMs >= 0.0f
					        ? otherClip.sourceEndMs
					        : static_cast<float>(context.sourceWaveformL.durationInSeconds *
					                             1000.0);
					const float otherDurationMs =
					    std::max(1.0f, otherSourceEndMs - otherClip.sourceStartMs);
					const float otherStartMs = otherClip.timelineStartMs;
					const float otherEndMs = otherClip.timelineStartMs + otherDurationMs;

					trySnapEdge(candidateStartMs, otherStartMs, 0.0f);
					trySnapEdge(candidateStartMs, otherEndMs, 0.0f);
					trySnapEdge(candidateEndMs, otherStartMs, movingDurationMs);
					trySnapEdge(candidateEndMs, otherEndMs, movingDurationMs);
				}

				clip->timelineStartMs = std::max(0.0f, bestStartMs);
				break;
			}
			case AudioDragMode::RangeSelect:
				audioRangeEndMs = currentTimelineMs;
				break;
			case AudioDragMode::TrimStart:
			{
				float trimDelta = deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary =
					    snapTimelineMs(context, audioDragStartClip.timelineStartMs + trimDelta,
					                   true);
					trimDelta = snappedBoundary - audioDragStartClip.timelineStartMs;
				}
				const float maxTrim = std::max(0.0f, startSourceEndMs - 1.0f);
				const float newStart = std::clamp(audioDragStartClip.sourceStartMs + trimDelta,
				                                  0.0f, maxTrim);
				const float appliedTrimDelta = newStart - audioDragStartClip.sourceStartMs;
				clip->sourceStartMs = newStart;
				clip->timelineStartMs = audioDragStartClip.timelineStartMs + appliedTrimDelta;
				break;
			}
			case AudioDragMode::TrimEnd:
			{
				float newEnd = startSourceEndMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary = snapTimelineMs(
					    context,
					    audioDragStartClip.timelineStartMs +
					        (newEnd - audioDragStartClip.sourceStartMs),
					    true);
					newEnd = audioDragStartClip.sourceStartMs +
					         (snappedBoundary - audioDragStartClip.timelineStartMs);
				}
				clip->sourceEndMs =
				    std::clamp(newEnd, clip->sourceStartMs + 1.0f, sourceLengthMs);
				break;
			}
			case AudioDragMode::FadeIn:
			{
				const float durationMs =
				    std::max(1.0f, (clip->sourceEndMs >= 0.0f ? clip->sourceEndMs : sourceLengthMs) -
				                         clip->sourceStartMs);
				float fadeInMs = audioDragStartClip.fadeInMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary =
					    snapTimelineMs(context, audioDragStartClip.timelineStartMs + fadeInMs,
					                   true);
					fadeInMs = snappedBoundary - audioDragStartClip.timelineStartMs;
				}
				clip->fadeInMs = std::clamp(fadeInMs, 0.0f, durationMs);
				break;
			}
			case AudioDragMode::FadeOut:
			{
				const float durationMs =
				    std::max(1.0f, (clip->sourceEndMs >= 0.0f ? clip->sourceEndMs : sourceLengthMs) -
				                         clip->sourceStartMs);
				float fadeOutMs = audioDragStartClip.fadeOutMs - deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary = snapTimelineMs(
					    context, audioDragStartClip.timelineStartMs + durationMs - fadeOutMs,
					    true);
					fadeOutMs = audioDragStartClip.timelineStartMs + durationMs - snappedBoundary;
				}
				clip->fadeOutMs = std::clamp(fadeOutMs, 0.0f, durationMs);
				break;
			}
			default:
				break;
			}

			if (audioDragMode != AudioDragMode::RangeSelect)
			{
				context.score.audioTrack.explicitEditorTrack = true;
				if (audioDragMode == AudioDragMode::Move || audioDragMode == AudioDragMode::TrimStart)
					AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				if (audioDragMode != AudioDragMode::RangeSelect)
				{
					context.pushHistory("Edit Audio Clip", audioDragStartScore, context.score);
					warnRefreshFailure(AudioTrackUtils::refreshPlaybackAudio(context));
				}
				audioDragMode = AudioDragMode::None;
				draggingAudioClip = static_cast<id_t>(-1);
			}
		}
	}

	void ScoreEditorTimeline::drawWaveform(ScoreContext& context)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		constexpr ImU32 waveformColorL = 0x80646464;
		constexpr ImU32 waveformColorR = 0x80585858;

		// Ideally this should be calculated based on the current BPM
		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const double durationSeconds = context.waveformL.durationInSeconds;
		const double musicOffsetInSeconds = context.workingData.musicOffset / 1000.0f;

		const float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());

		for (size_t index = 0; index < 2; index++)
		{
			const bool rightChannel = index == 1;
			const Audio::WaveformMipChain& waveform =
			    context.isAudioEditing()
			        ? (rightChannel ? context.sourceWaveformR : context.sourceWaveformL)
			        : (rightChannel ? context.waveformR : context.waveformL);
			if (waveform.isEmpty())
				continue;

			const ImU32 waveformColor = rightChannel ? waveformColorR : waveformColorL;
			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);

			for (int y = visualOffset - size.y; y < visualOffset; y += 1)
			{
				int tick = positionToTick(y);

				// Small accuracy loss by converting to ticks but shouldn't be too noticeable
				const double secondsAtPixel =
				    accumulateDuration(tick, TICKS_PER_BEAT, context.score.tempoChanges) -
				    musicOffsetInSeconds;
				const bool outOfBounds =
				    secondsAtPixel < 0 || secondsAtPixel > waveform.durationInSeconds;

				float amplitude =
				    std::max(waveform.getAmplitudeAt(mip, secondsAtPixel, secondsPerPixel), 0.0f);
				float barValue = outOfBounds ? 0.0f : (amplitude * std::min(laneWidth * 6, 180.0f));
				float rectYPosition = floorf(position.y + visualOffset - y);
				// WARNING: A thickness of 0.5 or less does not draw with integrated graphics
				// (optimization? limitation?)

				float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());
				ImVec2 rect1(timelineMidPosition, rectYPosition);
				ImVec2 rect2(timelineMidPosition +
				                 (std::max(0.75f, barValue) * (rightChannel ? 1 : -1)),
				             rectYPosition + 0.75f);
				drawList->AddRectFilled(rect1, rect2, waveformColor);
			}
		}
	}

}
