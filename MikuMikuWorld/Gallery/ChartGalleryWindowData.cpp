#include "ChartGalleryWindow.h"
#include "../Application.h"
#include "../Audio/Sound.h"
#include "../File.h"
#include "../IO.h"
#include "../Score/Serialization/NativeScoreSerializer.h"
#include "../ScoreStats.h"
#include "../Rendering/Texture.h"
#include "../Localization.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <future>
#include <iomanip>
#include <sstream>
#include <Windows.h>
namespace
{
	uint64_t fileTimeToUint64(const FILETIME& fileTime)
	{
		ULARGE_INTEGER value{};
		value.LowPart = fileTime.dwLowDateTime;
		value.HighPart = fileTime.dwHighDateTime;
		return value.QuadPart;
	}

	void getFileTimes(const std::string& filepath, uint64_t& createdTime, uint64_t& modifiedTime)
	{
		WIN32_FILE_ATTRIBUTE_DATA fileData{};
		if (!GetFileAttributesExW(IO::mbToWideStr(filepath).c_str(), GetFileExInfoStandard, &fileData))
			return;

		createdTime = fileTimeToUint64(fileData.ftCreationTime);
		modifiedTime = fileTimeToUint64(fileData.ftLastWriteTime);
	}

	std::string toLowerAscii(std::string s)
	{
		for (char& c : s) {
			if (c >= 'A' && c <= 'Z')
				c += ('a' - 'A');
		}
		return s;
	}

	int compareTextForSort(const std::string& left, const std::string& right)
	{
		std::wstring leftWide = IO::mbToWideStr(left);
		std::wstring rightWide = IO::mbToWideStr(right);
		int result = CompareStringEx(
			LOCALE_NAME_USER_DEFAULT,
			NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
			leftWide.c_str(),
			-1,
			rightWide.c_str(),
			-1,
			nullptr,
			nullptr,
			0);

		if (result == CSTR_LESS_THAN) return -1;
		if (result == CSTR_GREATER_THAN) return 1;
		if (result == CSTR_EQUAL) return 0;

		std::string leftKey = toLowerAscii(left);
		std::string rightKey = toLowerAscii(right);
		if (leftKey < rightKey) return -1;
		if (leftKey > rightKey) return 1;
		if (left < right) return -1;
		if (left > right) return 1;
		return 0;
	}
}

namespace MikuMikuWorld
{
std::shared_ptr<GalleryItem> ChartGalleryWindow::loadItemInfo(const std::string& filepath)
	{
		auto item = std::make_shared<GalleryItem>();
		item->filepath = filepath;
		item->filename = IO::File::getFilenameWithoutExtension(filepath);
		item->extension = IO::File::getFileExtension(filepath);
		item->title = "-"; 
		item->artist = "-";
		item->author = "-";
		item->folder = "-";
		getFileTimes(filepath, item->createdTime, item->modifiedTime);

		if (galleryStates.count(filepath)) {
			item->isFavorite = galleryStates[filepath].isFavorite;
			item->folder = galleryStates[filepath].folder;
		}

		try {
			Score tempScore = NativeScoreSerializer().deserialize(filepath);
			if (!tempScore.metadata.title.empty()) item->title = tempScore.metadata.title;
			if (!tempScore.metadata.artist.empty()) item->artist = tempScore.metadata.artist;
			if (!tempScore.metadata.author.empty()) item->author = tempScore.metadata.author;
			ScoreStats stats{};
			stats.calculateStats(tempScore);
			item->totalCombo = stats.getCombo();

			if (!tempScore.metadata.musicFile.empty()) {
				std::string absAudioPath = tempScore.metadata.musicFile;
				if (!IO::File::exists(absAudioPath)) {
					absAudioPath = IO::File::getFilepath(filepath) + tempScore.metadata.musicFile;
				}
				if (IO::File::exists(absAudioPath)) {
					Audio::SoundBuffer tempBuffer;
					if (Audio::decodeAudioFile(absAudioPath, tempBuffer).isOk()) {
						if (tempBuffer.sampleRate > 0) {
							double totalSeconds = static_cast<double>(tempBuffer.frameCount) / tempBuffer.sampleRate;
							int minutes = static_cast<int>(totalSeconds / 60);
							double seconds = totalSeconds - minutes * 60;
							std::ostringstream timeStream;
							timeStream << std::setfill('0') << std::setw(2) << minutes << ":"
									   << std::setfill('0') << std::fixed << std::setprecision(3) << seconds;
							item->lengthStr = timeStream.str();
						}
						tempBuffer.dispose();
					}
				}
			}

			item->jacketPath = tempScore.metadata.jacketFile;
			std::string absJacketPath = item->jacketPath;
			if (!absJacketPath.empty()) {
				if (!IO::File::exists(absJacketPath)) absJacketPath = IO::File::getFilepath(filepath) + item->jacketPath;
				if (IO::File::exists(absJacketPath)) item->texture = std::make_shared<Texture>(absJacketPath);
			}
		} catch (...) {}
		return item;
	}

void ChartGalleryWindow::refreshRecentItems(const std::vector<std::string>& recentFiles)
	{
		if (recentFiles == loadedRecentFiles)
			return;

		loadedRecentFiles = recentFiles;
		recentItems.clear();
		for (const std::string& filepath : recentFiles)
			if (IO::File::exists(filepath))
				recentItems.push_back(loadItemInfo(filepath));
	}

void ChartGalleryWindow::sortItems(std::vector<std::shared_ptr<GalleryItem>>& items)
	{
		auto isUnset = [](const std::string& value) {
			return value.empty() || value == "-";
		};

		auto applyDirection = [&](int comparison) {
			if (comparison == 0)
				return false;
			return sortAscending ? comparison < 0 : comparison > 0;
		};

		std::sort(items.begin(), items.end(), [&](const std::shared_ptr<GalleryItem>& a, const std::shared_ptr<GalleryItem>& b) {
			auto compareName = [&](const std::string& left, const std::string& right, const std::string& leftFallback, const std::string& rightFallback) {
				bool leftUnset = isUnset(left);
				bool rightUnset = isUnset(right);
				if (leftUnset != rightUnset)
					return !leftUnset;

				const std::string& leftValue = leftUnset ? leftFallback : left;
				const std::string& rightValue = rightUnset ? rightFallback : right;
				int valueComparison = compareTextForSort(leftValue, rightValue);
				if (valueComparison != 0)
					return applyDirection(valueComparison);

				int fallbackComparison = compareTextForSort(leftFallback, rightFallback);
				if (fallbackComparison != 0)
					return fallbackComparison < 0;

				return a->filepath < b->filepath;
			};

			auto compareNameNoUnset = [&](const std::string& left, const std::string& right) {
				int valueComparison = compareTextForSort(left, right);
				if (valueComparison != 0)
					return applyDirection(valueComparison);

				return a->filepath < b->filepath;
			};

			auto compareNumber = [&](uint64_t left, uint64_t right) {
				if (left != right)
					return sortAscending ? left < right : left > right;

				int filenameComparison = compareTextForSort(a->filename, b->filename);
				if (filenameComparison != 0)
					return filenameComparison < 0;

				return a->filepath < b->filepath;
			};

			if (sortMode == 0) return compareNumber(a->modifiedTime, b->modifiedTime);
			if (sortMode == 1) return compareNumber(a->createdTime, b->createdTime);
			if (sortMode == 3) return compareNumber(static_cast<uint64_t>(a->totalCombo), static_cast<uint64_t>(b->totalCombo));

			if (nameSortTarget == 0) return compareName(a->title, b->title, a->filename, b->filename);
			if (nameSortTarget == 1) return compareNameNoUnset(a->filename, b->filename);
			if (nameSortTarget == 2) return compareName(a->artist, b->artist, a->filename, b->filename);
			if (nameSortTarget == 3) return compareName(a->author, b->author, a->filename, b->filename);
			return a->filepath < b->filepath;
		});
	}

void ChartGalleryWindow::removeDeletedItemFromLists(const std::string& filepath)
	{
		auto removePredicate = [&](const std::shared_ptr<GalleryItem>& item) { return item->filepath == filepath; };
		recentItems.erase(std::remove_if(recentItems.begin(), recentItems.end(), removePredicate), recentItems.end());
		localItems.erase(std::remove_if(localItems.begin(), localItems.end(), removePredicate), localItems.end());
		galleryStates.erase(filepath);
		saveGalleryData();
	}

void ChartGalleryWindow::deleteFolder(int index)
	{
		std::string folderName = "";
		if (index == 3) folderName = "Team Projects";
		else if (index == 4) folderName = "Personal";
		else if (index >= 5 && (index - 5) < (int)customFolders.size()) folderName = customFolders[index - 5];

		if (folderName.empty()) return;

		for (auto& [path, state] : galleryStates) if (state.folder == folderName) state.folder = "-";
		for (auto& item : recentItems) if (item->folder == folderName) item->folder = "-";
		for (auto& item : localItems) if (item->folder == folderName) item->folder = "-";

		if (index >= 5) customFolders.erase(customFolders.begin() + (index - 5));

		activeTab = 0; 
		saveGalleryData();
	}

void ChartGalleryWindow::scanSearchPaths()
	{
		localItems.clear();
		try {
			for (const auto& pathStr : searchPaths) {
				std::wstring wp = IO::mbToWideStr(pathStr);
				if (std::filesystem::exists(wp)) {
					for (const auto& e : std::filesystem::recursive_directory_iterator(wp)) {
						if (e.is_regular_file()) {
							std::string ex = e.path().extension().string();
							if (ex == ".mmws" || ex == ".ccmmws" || ex == ".unchmmws" ||
							    ex == ".mchmmws") {
								localItems.push_back(loadItemInfo(IO::wideStringToMb(e.path().wstring())));
							}
						}
					}
				}
			}
		} catch (...) {}
	}

bool ChartGalleryWindow::addSearchPath(const std::string& path)
	{
		pathInputError.clear();

		if (path.empty())
			return false;

		std::filesystem::path folderPath = IO::mbToWideStr(path);
		std::error_code errorCode{};
		if (!std::filesystem::exists(folderPath, errorCode) || !std::filesystem::is_directory(folderPath, errorCode)) {
			pathInputError = getString("gallery_invalid_search_path");
			return false;
		}

		if (std::find(searchPaths.begin(), searchPaths.end(), path) == searchPaths.end()) {
			searchPaths.push_back(path);
			saveGalleryData();
		}

		scanSearchPaths();
		return true;
	}

void ChartGalleryWindow::startFolderDialog()
	{
		if (folderDialogInProgress)
			return;

		std::string title = getString("gallery_browse_folder");
		void* parentWindowHandle = Application::windowState.windowHandle;
		folderDialogInProgress = true;
		folderDialogFuture = std::async(std::launch::async, [title, parentWindowHandle]() {
			IO::FileDialog fileDialog{};
			fileDialog.title = title;
			fileDialog.parentWindowHandle = parentWindowHandle;

			if (fileDialog.openFolder() == IO::FileDialogResult::OK)
				return fileDialog.outputFilename;

			return std::string{};
		});
	}

void ChartGalleryWindow::pollFolderDialog()
	{
		if (!folderDialogInProgress || !folderDialogFuture.valid())
			return;

		if (folderDialogFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return;

		std::string selectedPath = folderDialogFuture.get();
		folderDialogInProgress = false;

		if (!selectedPath.empty())
			addSearchPath(selectedPath);
	}
}
