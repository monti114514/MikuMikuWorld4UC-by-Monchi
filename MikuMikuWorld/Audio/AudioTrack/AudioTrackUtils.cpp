#include "../../AudioTrackUtils.h"
#include <cmath>

namespace MikuMikuWorld::AudioTrackUtils
{
	void ensureDefaultAudioTrack(Score& score, const std::string& sourceFile, float timelineStartMs,
	                             float sourceLengthMs)
	{
		if (score.audioTrack.explicitEditorTrack)
			return;
		if (!score.audioTrack.clips.empty())
			return;

		AudioClip clip;
		clip.ID = getNextAudioClipID();
		clip.sourceFile = sourceFile;
		clip.sourceStartMs = 0.0f;
		clip.sourceEndMs = -1.0f;
		clip.timelineStartMs = timelineStartMs;
		score.audioTrack.clips.push_back(clip);
	}

	bool hasAudioTrackData(const Score& score)
	{
		return score.audioTrack.explicitEditorTrack || !score.audioTrack.clips.empty();
	}

	bool hasAudioTrackEdits(const Score& score)
	{
		if (score.audioTrack.explicitEditorTrack && score.audioTrack.clips.empty())
			return true;
		if (score.audioTrack.clips.size() != 1)
			return !score.audioTrack.clips.empty();
		if (score.audioTrack.clips.empty())
			return false;

		const AudioClip& clip = score.audioTrack.clips.front();
		if (std::abs(clip.timelineStartMs - score.metadata.musicOffset) > 0.01f)
			return true;
		if (clip.sourceStartMs > 0.01f || clip.fadeInMs > 0.01f || clip.fadeOutMs > 0.01f)
			return true;
		if (std::abs(clip.gain - 1.0f) > 0.001f)
			return true;
		if (!clip.sourceFile.empty() && clip.sourceFile != score.metadata.musicFile)
			return true;
		return clip.sourceEndMs >= 0.0f;
	}
}
