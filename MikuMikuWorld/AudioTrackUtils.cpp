#include "AudioTrackUtils.h"
#include "Application.h"
#include "File.h"
#include "IO.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

#define NOMINMAX
#include <Windows.h>

namespace MikuMikuWorld::AudioTrackUtils
{
	namespace fs = std::filesystem;

	static float getClipSourceEndMs(const AudioClip& clip, const Audio::SoundBuffer& source)
	{
		const float sourceLengthMs =
		    source.sampleRate > 0 ? (static_cast<float>(source.frameCount) / source.sampleRate) * 1000.0f
		                          : 0.0f;
		if (clip.sourceEndMs < 0.0f)
			return sourceLengthMs;
		return std::clamp(clip.sourceEndMs, 0.0f, sourceLengthMs);
	}

	static std::string resolveSourceFile(const AudioClip& clip, const std::string& fallbackSourceFile)
	{
		return clip.sourceFile.empty() ? fallbackSourceFile : clip.sourceFile;
	}

	static void writeU16(std::ofstream& stream, uint16_t value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	static void writeU32(std::ofstream& stream, uint32_t value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	static std::string quoteArgument(const std::string& value)
	{
		std::string quoted = "\"";
		for (char c : value)
		{
			if (c == '"')
				quoted += "\\\"";
			else
				quoted += c;
		}
		quoted += "\"";
		return quoted;
	}

	static std::string findFfmpeg()
	{
		const std::string appDir = Application::getAppDir();
		const std::vector<std::string> bundledCandidates{
			appDir + "ffmpeg.exe",
			appDir + "tools\\ffmpeg.exe",
			appDir + "res\\tools\\ffmpeg.exe",
			appDir + "..\\Depends\\ffmpeg\\bin\\ffmpeg.exe"
		};

		for (const std::string& candidate : bundledCandidates)
		{
			if (IO::File::exists(candidate))
				return candidate;
		}

		wchar_t pathBuffer[MAX_PATH]{};
		if (SearchPathW(nullptr, L"ffmpeg.exe", nullptr, MAX_PATH, pathBuffer, nullptr) > 0)
			return IO::wideStringToMb(std::wstring(pathBuffer));

		return {};
	}

	static Result runProcess(const std::string& commandLine)
	{
		std::wstring commandLineW = IO::mbToWideStr(commandLine);
		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};

		if (!CreateProcessW(nullptr, commandLineW.data(), nullptr, nullptr, FALSE,
		                    CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
		{
			return Result(ResultStatus::Error, "Failed to start ffmpeg process");
		}

		WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode = 1;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);

		if (exitCode != 0)
			return Result(ResultStatus::Error,
			              IO::formatString("ffmpeg exited with code %lu", exitCode));

		return Result::Ok();
	}

	void ensureDefaultAudioTrack(Score& score, const std::string& sourceFile, float timelineStartMs,
	                             float sourceLengthMs)
	{
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
		return !score.audioTrack.clips.empty();
	}

	bool hasAudioTrackEdits(const Score& score)
	{
		if (score.audioTrack.muted || score.audioTrack.locked || !score.audioTrack.visible)
			return true;
		if (score.audioTrack.clips.size() != 1)
			return !score.audioTrack.clips.empty();
		if (score.audioTrack.clips.empty())
			return false;

		const AudioClip& clip = score.audioTrack.clips.front();
		if (clip.muted || clip.locked || !clip.visible)
			return true;
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

	float getRenderedTimelineStartMs(const Score& score, float fallbackMusicOffsetMs)
	{
		float startMs = std::numeric_limits<float>::max();
		for (const AudioClip& clip : score.audioTrack.clips)
		{
			if (clip.muted || score.audioTrack.muted)
				continue;
			startMs = std::min(startMs, clip.timelineStartMs);
		}
		return startMs == std::numeric_limits<float>::max() ? fallbackMusicOffsetMs : startMs;
	}

	Result renderToBuffer(const Score& score, const std::string& fallbackSourceFile,
	                      RenderedAudio& output)
	{
		if (score.audioTrack.clips.empty())
			return Result(ResultStatus::Error, "No audio clips to render");

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
			if (score.audioTrack.muted || clip.muted || !clip.visible)
				continue;

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

	Result writeTempWav(const Audio::SoundBuffer& buffer, const std::string& filename)
	{
		if (!buffer.isValid())
			return Result(ResultStatus::Error, "Invalid audio buffer");

		std::ofstream stream(IO::mbToWideStr(filename), std::ios::binary);
		if (!stream.is_open())
			return Result(ResultStatus::Error, "Failed to write temporary wav file");

		const uint16_t bitsPerSample = 16;
		const uint16_t blockAlign = static_cast<uint16_t>(buffer.channelCount * bitsPerSample / 8);
		const uint32_t byteRate = buffer.sampleRate * blockAlign;
		const uint32_t dataSize =
		    static_cast<uint32_t>(buffer.frameCount * buffer.channelCount * sizeof(int16_t));

		stream.write("RIFF", 4);
		writeU32(stream, 36 + dataSize);
		stream.write("WAVE", 4);
		stream.write("fmt ", 4);
		writeU32(stream, 16);
		writeU16(stream, 1);
		writeU16(stream, static_cast<uint16_t>(buffer.channelCount));
		writeU32(stream, buffer.sampleRate);
		writeU32(stream, byteRate);
		writeU16(stream, blockAlign);
		writeU16(stream, bitsPerSample);
		stream.write("data", 4);
		writeU32(stream, dataSize);
		stream.write(reinterpret_cast<const char*>(buffer.samples.get()), dataSize);
		return Result::Ok();
	}

	std::string makeEditedAudioFilename(const std::string& sourceFile,
	                                    const std::string& outputDirectory)
	{
		const std::string sourceName =
		    sourceFile.empty() ? "bgm" : IO::File::getFilenameWithoutExtension(sourceFile);
		const std::string extension = sourceFile.empty() ? ".wav" : IO::File::getFileExtension(sourceFile);
		return (fs::path(outputDirectory) / (sourceName + "_edited" + extension)).u8string();
	}

	Result exportEditedAudio(const Score& score, const std::string& fallbackSourceFile,
	                         const std::string& outputDirectory, std::string& outputFilename)
	{
		RenderedAudio rendered;
		Result renderResult = renderToBuffer(score, fallbackSourceFile, rendered);
		if (!renderResult.isOk())
			return renderResult;

		std::string sourceFile = fallbackSourceFile;
		for (const AudioClip& clip : score.audioTrack.clips)
		{
			if (!clip.sourceFile.empty())
			{
				sourceFile = clip.sourceFile;
				break;
			}
		}

		outputFilename = makeEditedAudioFilename(sourceFile, outputDirectory);
		const fs::path tempWavPath = fs::path(outputDirectory) / "__mmw4uc_audio_edit_temp.wav";
		Result wavResult = writeTempWav(rendered.buffer, tempWavPath.u8string());
		if (!wavResult.isOk())
			return wavResult;

		const std::string ffmpeg = findFfmpeg();
		if (ffmpeg.empty())
			return Result(ResultStatus::Error,
			              "ffmpeg.exe was not found. Place it next to the application or in tools.");

		const std::string command = quoteArgument(ffmpeg) + " -y -hide_banner -loglevel error -i " +
		                            quoteArgument(tempWavPath.u8string()) + " " +
		                            quoteArgument(outputFilename);
		Result ffmpegResult = runProcess(command);
		std::error_code ec;
		fs::remove(tempWavPath, ec);
		return ffmpegResult;
	}
}
