#include "../../AudioTrackUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace MikuMikuWorld::AudioTrackUtils
{
	namespace
	{
		float getClipSourceEndMs(const AudioClip& clip, const Audio::SoundBuffer& source)
		{
			const float sourceLengthMs =
			    source.sampleRate > 0 ? (static_cast<float>(source.frameCount) / source.sampleRate) * 1000.0f
			                          : 0.0f;
			if (clip.sourceEndMs < 0.0f)
				return sourceLengthMs;
			return std::clamp(clip.sourceEndMs, 0.0f, sourceLengthMs);
		}

		std::string resolveSourceFile(const AudioClip& clip, const std::string& fallbackSourceFile)
		{
			return clip.sourceFile.empty() ? fallbackSourceFile : clip.sourceFile;
		}
	}

	float getRenderedTimelineStartMs(const Score& score, float fallbackMusicOffsetMs,
	                                 bool ignoreEditorMute)
	{
		float startMs = std::numeric_limits<float>::max();
		for (const AudioClip& clip : score.audioTrack.clips)
			startMs = std::min(startMs, clip.timelineStartMs);
		return startMs == std::numeric_limits<float>::max() ? fallbackMusicOffsetMs : startMs;
	}

	Result renderToBuffer(const Score& score, const std::string& fallbackSourceFile,
	                      RenderedAudio& output, bool ignoreEditorMute)
	{
		if (score.audioTrack.clips.empty())
		{
			if (!score.audioTrack.explicitEditorTrack)
				return Result(ResultStatus::Error, "No audio clips to render");

			const ma_uint32 sampleRate = 44100;
			const ma_uint32 channelCount = 2;
			const ma_uint64 frameCount = 1;
			int16_t* samples = new int16_t[channelCount * frameCount]{};
			output.buffer.initialize("silence", sampleRate, channelCount, frameCount, samples);
			output.timelineStartMs = score.metadata.musicOffset;
			return Result::Ok();
		}

		struct DecodedClip
		{
			AudioClip clip;
			Audio::SoundBuffer source;
			float sourceEndMs{};
		};

		std::vector<DecodedClip> decodedClips;
		float renderStartMs = std::numeric_limits<float>::max();
		float renderEndMs = -std::numeric_limits<float>::max();
		uint32_t sampleRate = 0;
		uint32_t channelCount = 0;

		for (const AudioClip& clip : score.audioTrack.clips)
		{
			const std::string sourceFile = resolveSourceFile(clip, fallbackSourceFile);
			if (sourceFile.empty())
				continue;

			DecodedClip decoded;
			decoded.clip = clip;
			Result result = Audio::decodeAudioFile(sourceFile, decoded.source);
			if (!result.isOk())
				return result;

			if (sampleRate == 0)
			{
				sampleRate = decoded.source.sampleRate;
				channelCount = decoded.source.channelCount;
			}
			else if (sampleRate != decoded.source.sampleRate ||
			         channelCount != decoded.source.channelCount)
			{
				return Result(ResultStatus::Error,
				              "Audio clips must use the same sample rate and channel count");
			}

			decoded.sourceEndMs = getClipSourceEndMs(decoded.clip, decoded.source);
			const float sourceDurationMs =
			    std::max(0.0f, decoded.sourceEndMs - decoded.clip.sourceStartMs);
			if (sourceDurationMs <= 0.0f)
				continue;

			renderStartMs = std::min(renderStartMs, decoded.clip.timelineStartMs);
			renderEndMs = std::max(renderEndMs, decoded.clip.timelineStartMs + sourceDurationMs);
			decodedClips.push_back(std::move(decoded));
		}

		if (decodedClips.empty() || renderEndMs <= renderStartMs || sampleRate == 0 || channelCount == 0)
			return Result(ResultStatus::Error, "No audible audio clips to render");

		const uint64_t outputFrames =
		    static_cast<uint64_t>(std::ceil(((renderEndMs - renderStartMs) / 1000.0f) * sampleRate));
		const uint64_t outputSamples = outputFrames * channelCount;
		int16_t* samples = new int16_t[static_cast<size_t>(outputSamples)]{};

		for (const DecodedClip& decoded : decodedClips)
		{
			const AudioClip& clip = decoded.clip;
			const uint64_t sourceStartFrame =
			    static_cast<uint64_t>(std::max(0.0f, clip.sourceStartMs) / 1000.0f *
			                          decoded.source.sampleRate);
			const uint64_t sourceEndFrame =
			    static_cast<uint64_t>(decoded.sourceEndMs / 1000.0f * decoded.source.sampleRate);
			const uint64_t destinationStartFrame =
			    static_cast<uint64_t>((clip.timelineStartMs - renderStartMs) / 1000.0f * sampleRate);
			const uint64_t frameCount =
			    std::min<uint64_t>(sourceEndFrame - sourceStartFrame,
			                       outputFrames - destinationStartFrame);

			for (uint64_t frame = 0; frame < frameCount; ++frame)
			{
				const float clipPositionMs = static_cast<float>(frame) / sampleRate * 1000.0f;
				float gain = clip.gain;
				if (clip.fadeInMs > 0.0f && clipPositionMs < clip.fadeInMs)
					gain *= clipPositionMs / clip.fadeInMs;
				if (clip.fadeOutMs > 0.0f)
				{
					const float clipDurationMs =
					    static_cast<float>(frameCount) / sampleRate * 1000.0f;
					const float fadeOutStartMs = std::max(0.0f, clipDurationMs - clip.fadeOutMs);
					if (clipPositionMs > fadeOutStartMs)
						gain *= std::clamp((clipDurationMs - clipPositionMs) / clip.fadeOutMs,
						                   0.0f, 1.0f);
				}

				for (uint32_t channel = 0; channel < channelCount; ++channel)
				{
					const uint64_t srcIndex = ((sourceStartFrame + frame) * channelCount) + channel;
					const uint64_t dstIndex =
					    ((destinationStartFrame + frame) * channelCount) + channel;
					const int mixed =
					    static_cast<int>(samples[dstIndex]) +
					    static_cast<int>(decoded.source.samples[srcIndex] * gain);
					samples[dstIndex] =
					    static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
				}
			}
		}

		output.buffer.initialize("edited_bgm", sampleRate, channelCount, outputFrames, samples);
		output.timelineStartMs = renderStartMs;
		return Result::Ok();
	}
}
