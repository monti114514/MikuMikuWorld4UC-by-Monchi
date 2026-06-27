#include "ChartGalleryWindow.h"
#include "../Application.h"
#include "../Constants.h"
#include "../IO.h"
#include <fstream>
#include "json.hpp"
namespace MikuMikuWorld
{
	void ChartGalleryWindow::loadGalleryData()
	{
		auto applyGalleryConfig = [&](const GalleryConfiguration& galleryConfig) {
			galleryStates = galleryConfig.charts;
			customFolders = galleryConfig.folders;
			searchPaths = galleryConfig.searchPaths;
			isSearchPathsOpen = galleryConfig.isSearchPathsOpen;
		};

		if (config.hasGallerySettings)
		{
			applyGalleryConfig(config.gallery);
			return;
		}

		GalleryConfiguration galleryConfig{};
		std::string path = Application::getAppDir() + "gallery_config.json";
		std::ifstream file(IO::mbToWideStr(path));
		if (file.is_open()) {
			nlohmann::json j;
			try {
				file >> j;
				if (j.contains("charts")) {
					for (auto& [filepath, data] : j["charts"].items()) {
						galleryConfig.charts[filepath] = {
							data.value("isFavorite", false),
							data.value("folder", "-")
						};
					}
				}
				if (j.contains("folders")) {
					for (auto& folder : j["folders"])
						galleryConfig.folders.push_back(folder.get<std::string>());
				}
				if (j.contains("searchPaths")) {
					for (auto& p : j["searchPaths"])
						galleryConfig.searchPaths.push_back(p.get<std::string>());
				} else if (j.contains("searchPath")) {
					galleryConfig.searchPaths.push_back(j["searchPath"].get<std::string>());
				}
				if (j.contains("isSearchPathsOpen"))
					galleryConfig.isSearchPathsOpen = j["isSearchPathsOpen"].get<bool>();
			} catch (...) {}
		}

		config.gallery = galleryConfig;
		config.hasGallerySettings = true;
		applyGalleryConfig(config.gallery);
	}

	void ChartGalleryWindow::saveGalleryData()
	{
		config.gallery.charts = galleryStates;
		config.gallery.folders = customFolders;
		config.gallery.searchPaths = searchPaths;
		config.gallery.isSearchPathsOpen = isSearchPathsOpen;
		config.hasGallerySettings = true;
		config.write(Application::getAppDir() + APP_CONFIG_FILENAME);
	}
}
