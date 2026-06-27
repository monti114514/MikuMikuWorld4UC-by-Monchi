#include "../../AudioTrackUtils.h"
#include "../../Application.h"
#include "../../File.h"
#include "../../IO.h"
#include <filesystem>
#include <fstream>
#include <vector>

#define NOMINMAX
#include <Windows.h>

namespace MikuMikuWorld::AudioTrackUtils
{
	namespace fs = std::filesystem;

	namespace
	{
		void writeU16(std::ofstream& stream, uint16_t value)
		{
			stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
		}

		void writeU32(std::ofstream& stream, uint32_t value)
		{
			stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
		}

		std::string quoteArgument(const std::string& value)
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

		std::string findFfmpeg()
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

		Result runProcess(const std::string& commandLine)
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

		fs::path makeTemporaryWavPath()
		{
			wchar_t tempDirectory[MAX_PATH]{};
			if (GetTempPathW(MAX_PATH, tempDirectory) == 0)
			{
				std::error_code ec;
				fs::path fallback = fs::temp_directory_path(ec);
				if (!ec)
					return fallback / "__mmw4uc_audio_edit_temp.wav";
				return fs::path("__mmw4uc_audio_edit_temp.wav");
			}

			wchar_t tempFilename[MAX_PATH]{};
			if (GetTempFileNameW(tempDirectory, L"mmw", 0, tempFilename) == 0)
				return fs::path(tempDirectory) / "__mmw4uc_audio_edit_temp.wav";

			fs::path path(tempFilename);
			std::error_code ec;
			fs::remove(path, ec);
			path.replace_extension(L".wav");
			return path;
		}

		Result writeTempWav(const Audio::SoundBuffer& buffer, const fs::path& filename)
		{
			if (!buffer.isValid())
				return Result(ResultStatus::Error, "Invalid audio buffer");

			std::ofstream stream(filename, std::ios::binary);
			if (!stream.is_open())
				return Result(ResultStatus::Error,
				              IO::formatString("Failed to write temporary wav file: %s",
				                               IO::wideStringToMb(filename.wstring()).c_str()));

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

		std::string pathToUtf8(const fs::path& path)
		{
			return IO::wideStringToMb(path.wstring());
		}

		fs::path makeEditedAudioPath(const std::string& sourceFile,
		                             const fs::path& outputDirectory)
		{
			std::wstring sourceName = L"bgm";
			std::wstring extension = L".wav";
			if (!sourceFile.empty())
			{
				const fs::path sourcePath(IO::mbToWideStr(sourceFile));
				if (!sourcePath.stem().wstring().empty())
					sourceName = sourcePath.stem().wstring();
				if (!sourcePath.extension().wstring().empty())
					extension = sourcePath.extension().wstring();
			}

			return outputDirectory / (sourceName + L"_edited" + extension);
		}
	}

	Result exportEditedAudio(const Score& score, const std::string& fallbackSourceFile,
	                         const std::string& outputDirectory, std::string& outputFilename)
	{
		RenderedAudio rendered;
		Result renderResult = renderToBuffer(score, fallbackSourceFile, rendered, true);
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

		const fs::path outputDirectoryPath =
		    outputDirectory.empty() ? fs::current_path() : fs::path(IO::mbToWideStr(outputDirectory));
		std::error_code ec;
		fs::create_directories(outputDirectoryPath, ec);
		if (ec)
			return Result(ResultStatus::Error,
			              IO::formatString("Failed to create output directory: %s",
			                               pathToUtf8(outputDirectoryPath).c_str()));

		const fs::path outputPath = makeEditedAudioPath(sourceFile, outputDirectoryPath);
		outputFilename = pathToUtf8(outputPath);
		const fs::path tempWavPath = makeTemporaryWavPath();
		Result wavResult = writeTempWav(rendered.buffer, tempWavPath);
		if (!wavResult.isOk())
			return wavResult;

		const std::string ffmpeg = findFfmpeg();
		if (ffmpeg.empty())
			return Result(ResultStatus::Error,
			              "ffmpeg.exe was not found. Place it next to the application or in tools.");

		const std::string command = quoteArgument(ffmpeg) + " -y -hide_banner -loglevel error -i " +
		                            quoteArgument(pathToUtf8(tempWavPath)) + " " +
		                            quoteArgument(outputFilename);
		Result ffmpegResult = runProcess(command);
		std::error_code removeError;
		fs::remove(tempWavPath, removeError);
		return ffmpegResult;
	}
}
