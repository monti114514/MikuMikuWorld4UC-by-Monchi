#include "ScoreEditorTimeline.h"
#include "../../ApplicationConfiguration.h"
#include "../../Audio/Sound.h"
#include "../../Tempo.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace MikuMikuWorld
{
	namespace
	{
		const HiSpeedChange* findActiveLayerHiSpeed(const Score& score, int layer, int tick)
		{
			const HiSpeedChange* active = nullptr;
			for (const auto& [id, hiSpeed] : score.hiSpeedChanges)
			{
				if (hiSpeed.layer != layer || hiSpeed.tick > tick)
					continue;

				if (!active || hiSpeed.tick > active->tick ||
				    (hiSpeed.tick == active->tick && hiSpeed.ID > active->ID))
					active = &hiSpeed;
			}

			return active;
		}

		bool isHideNotesActive(const Score& score, int layer, int tick)
		{
			if (layer < 0 || layer >= static_cast<int>(score.layers.size()))
				return false;

			const HiSpeedChange* active = findActiveLayerHiSpeed(score, layer, tick);
			return active && active->hideNotes;
		}

		std::vector<int> getHiSpeedBoundaryTicks(const Score& score, int layer, int startTick, int endTick)
		{
			std::vector<int> boundaries{ startTick, endTick };
			for (const auto& [id, hiSpeed] : score.hiSpeedChanges)
			{
				if (hiSpeed.layer == layer && hiSpeed.tick > startTick && hiSpeed.tick < endTick)
					boundaries.push_back(hiSpeed.tick);
			}

			std::sort(boundaries.begin(), boundaries.end());
			boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
			return boundaries;
		}


	}

	void ScoreEditorTimeline::setPlaybackSpeed(ScoreContext& context, float speed)
	{
		playbackSpeed = std::clamp(speed, minPlaybackSpeed, maxPlaybackSpeed);
		context.audio.setPlaybackSpeed(playbackSpeed, time);
	}

	void ScoreEditorTimeline::setPlaying(ScoreContext& context, bool state)
	{
		if (playing == state)
			return;

		playing = state;
		if (playing)
		{
			playStartTime = time;
			context.audio.seekMusic(time);
			context.audio.playMusic(time);
			context.audio.setLastPlaybackTime(time);
			context.audio.syncAudioEngineTimer();
		}
		else
		{
			if (config.returnToLastSelectedTickOnPause)
			{
				context.currentTick = lastSelectedTick;
				offset =
				    std::max(minOffset, tickToPosition(context.currentTick) +
				                            (size.y * (1.0f - config.cursorPositionThreshold)));
			}

			context.audio.stopSoundEffects(false);
			context.audio.stopMusic();
		}
	}

	void ScoreEditorTimeline::stop(ScoreContext& context)
	{
		playing = false;
		time = lastSelectedTick = context.currentTick = 0;
		offset = std::max(minOffset, tickToPosition(context.currentTick) +
		                                 (size.y * (1.0f - config.cursorPositionThreshold)));

		context.audio.stopSoundEffects(false);
		context.audio.stopMusic();
	}

	void ScoreEditorTimeline::updateNoteSE(ScoreContext& context)
	{
		if (!playing)
			return;

		auto singleNoteSEFunc = [&context, this](const Note& note, float notePlayTime)
		{
			bool playSE = true;
			if (note.getType() == NoteType::Hold)
			{
				playSE = context.score.holdNotes.at(note.ID).startType == HoldNoteType::Normal;
			}
			else if (note.getType() == NoteType::HoldEnd)
            {
                if (context.score.holdNotes.find(note.parentID) != context.score.holdNotes.end()) 
                {
                    playSE = context.score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
                }
                else 
                {
                    playSE = false;
                }
            }
			if (note.isHold())
			{
				const id_t holdId = note.getType() == NoteType::Hold ? note.ID : note.parentID;
				const auto holdIt = context.score.holdNotes.find(holdId);
				if (holdIt != context.score.holdNotes.end() && holdIt->second.dummy)
					playSE = false;
			}
			if (note.dummy)
			{
				playSE = false;
			}

			if (playSE)
			{
				std::string_view se = getNoteSE(note, context.score);
				std::string key = std::to_string(note.tick) + "-" + se.data();
				if (!se.empty() && (playingNoteSounds.find(key) == playingNoteSounds.end()))
				{
					context.audio.playSoundEffect(se.data(), notePlayTime, -1, time);
					playingNoteSounds.insert(key);
				}
			}
		};

		auto holdNoteSEFunc = [&context, this](const Note& note, float startTime)
		{
			HoldNote& hold = context.score.holdNotes.at(note.ID);
			if (hold.dummy)
				return;
			int endTick = context.score.notes.at(hold.end).tick;
			if (endTick <= note.tick)
				return;

			const std::string_view se = note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT;
			const std::vector<int> boundaries =
			    getHiSpeedBoundaryTicks(context.score, note.layer, note.tick, endTick);
			for (size_t i = 0; i + 1 < boundaries.size(); ++i)
			{
				const int segmentStartTick = boundaries[i];
				const int segmentEndTick = boundaries[i + 1];
				if (isHideNotesActive(context.score, note.layer, segmentStartTick))
					continue;

				const float segmentStartTime =
				    accumulateDuration(segmentStartTick, TICKS_PER_BEAT, context.score.tempoChanges);
				const float segmentEndTime =
				    accumulateDuration(segmentEndTick, TICKS_PER_BEAT, context.score.tempoChanges);
				const float adjustedStartTime =
				    std::max(startTime, segmentStartTime - playStartTime - audioOffsetCorrection);
				const float adjustedEndTime = segmentEndTime - playStartTime + audioOffsetCorrection;
				if (adjustedEndTime <= adjustedStartTime)
					continue;

				const float engineTime = context.audio.getAudioEngineAbsoluteTime();
				const float scheduledEndTime =
				    adjustedEndTime + playbackSpeed * (adjustedStartTime - engineTime);
				context.audio.playSoundEffect(se.data(), adjustedStartTime, scheduledEndTime, time);
			}
		};

		playingNoteSounds.clear();
		for (const auto& [id, note] : context.score.notes)
		{
			float noteTime =
			    accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float notePlayTime = noteTime - playStartTime;
			float offsetNoteTime = noteTime - (audioLookAhead * playbackSpeed);

			if (offsetNoteTime >= timeLastFrame && offsetNoteTime < time)
			{
				singleNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
					holdNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
			}
			else if (time == playStartTime)
			{
				// Playback just started
				if (noteTime >= time && offsetNoteTime < time)
					singleNoteSEFunc(note, notePlayTime);

				// Playback started mid-hold
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
				{
					int endTick =
					    context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
					float endTime =
					    accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);
					if ((noteTime - time) <= audioLookAhead && endTime > time)
						holdNoteSEFunc(note, std::max(0.0f, notePlayTime));
				}
			}
		}
	}


}
