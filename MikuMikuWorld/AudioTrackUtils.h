#pragma once
#include "Audio/Sound.h"
#include "Score.h"
#include "Utilities.h"
#include <string>

namespace MikuMikuWorld::AudioTrackUtils
{
	struct RenderedAudio
	{
		Audio::SoundBuffer buffer;
		float timelineStartMs = 0.0f;
	};

	void ensureDefaultAudioTrack(Score& score, const std::string& sourceFile,
	                             float timelineStartMs, float sourceLengthMs);
	bool hasAudioTrackData(const Score& score);
	bool hasAudioTrackEdits(const Score& score);
	float getRenderedTimelineStartMs(const Score& score, float fallbackMusicOffsetMs);
	Result renderToBuffer(const Score& score, const std::string& fallbackSourceFile,
	                      RenderedAudio& output);
	Result writeTempWav(const Audio::SoundBuffer& buffer, const std::string& filename);
	std::string makeEditedAudioFilename(const std::string& sourceFile,
	                                    const std::string& outputDirectory);
	Result exportEditedAudio(const Score& score, const std::string& fallbackSourceFile,
	                         const std::string& outputDirectory, std::string& outputFilename);
}
