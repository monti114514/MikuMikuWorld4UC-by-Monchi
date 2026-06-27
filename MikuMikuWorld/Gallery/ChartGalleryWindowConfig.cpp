#include "ChartGalleryWindow.h"
#include "../Application.h"
#include "../Constants.h"
namespace MikuMikuWorld
{
	void ChartGalleryWindow::loadGalleryData()
	{
		galleryStates = config.gallery.charts;
		customFolders = config.gallery.folders;
		searchPaths = config.gallery.searchPaths;
		isSearchPathsOpen = config.gallery.isSearchPathsOpen;
	}

	void ChartGalleryWindow::saveGalleryData()
	{
		config.gallery.charts = galleryStates;
		config.gallery.folders = customFolders;
		config.gallery.searchPaths = searchPaths;
		config.gallery.isSearchPathsOpen = isSearchPathsOpen;
		config.write(Application::getAppDir() + APP_CONFIG_FILENAME);
	}
}
