#include "../../AudioTrackUtils.h"
#include "../../ScoreEditor/Context/ScoreContext.h"
#include <limits>

namespace MikuMikuWorld::AudioTrackUtils
{
	namespace
	{
		Result loadSilence(ScoreContext& context, float timelineStartMs)
		{
			const ma_uint32 sampleRate = 44100;
			const ma_uint32 channelCount = 2;
			const ma_uint64 frameCount = 1;
			int16_t* samples = new int16_t[channelCount * frameCount]{};
			Result result =
			    context.audio.loadMusicFromSamples("silence", sampleRate, channelCount, frameCount, samples);
			if (!result.isOk())
				return result;
			context.audio.setMusicOffset(0.0f, timelineStartMs);
			context.waveformL.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 0);
			context.waveformR.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 1);
			return Result::Ok();
		}
	}

	Result refreshPlaybackAudio(ScoreContext& context)
	{
		if (hasAudioTrackData(context.score))
		{
			if (context.score.audioTrack.clips.empty())
				return loadSilence(context, context.workingData.musicOffset);

			if (hasAudioTrackEdits(context.score))
			{
				RenderedAudio rendered;
				Result renderResult =
				    renderToBuffer(context.score, context.workingData.musicFilename, rendered);
				if (!renderResult.isOk())
					return loadSilence(context, context.workingData.musicOffset);

				int16_t* samples = rendered.buffer.samples.release();
				const std::string name = rendered.buffer.name;
				const ma_uint32 sampleRate = rendered.buffer.sampleRate;
				const ma_uint32 channelCount = rendered.buffer.channelCount;
				const ma_uint64 frameCount = rendered.buffer.frameCount;
				rendered.buffer.dispose();

				Result loadResult =
				    context.audio.loadMusicFromSamples(name, sampleRate, channelCount, frameCount, samples);
				if (!loadResult.isOk())
					return loadResult;

				context.audio.setMusicOffset(0.0f, rendered.timelineStartMs);
				context.waveformL.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 0);
				context.waveformR.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 1);
				return Result::Ok();
			}
		}

		if (context.workingData.musicFilename.empty())
		{
			context.audio.disposeMusic();
			context.waveformL.clear();
			context.waveformR.clear();
			return Result::Ok();
		}

		Result result = context.audio.loadMusic(context.workingData.musicFilename);
		if (!result.isOk())
			return result;
		context.audio.setMusicOffset(0.0f, context.workingData.musicOffset);
		context.waveformL.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 0);
		context.waveformR.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 1);
		return Result::Ok();
	}

	void syncMusicOffsetFromAudioTrack(ScoreContext& context)
	{
		if (context.score.audioTrack.clips.empty())
			return;

		float startMs = std::numeric_limits<float>::max();
		for (const AudioClip& clip : context.score.audioTrack.clips)
			startMs = std::min(startMs, clip.timelineStartMs);
		if (startMs == std::numeric_limits<float>::max())
			return;

		context.workingData.musicOffset = startMs;
		context.score.metadata.musicOffset = startMs;
	}
}
