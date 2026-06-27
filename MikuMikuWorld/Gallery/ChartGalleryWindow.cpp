#include "ChartGalleryWindow.h"
#include "../Application.h"
#include "../File.h"
#include "../UI.h"

namespace MikuMikuWorld
{
	void ChartGalleryWindow::update(const std::vector<std::string>& recentFiles)
	{
		if (!open) return;
	
		if (!recentLoaded) {
			loadGalleryData();
			std::string iconPath = Application::getAppDir() + "res\\textures\\gallery\\placeholder.png";
			if (IO::File::exists(iconPath)) defaultIcon = std::make_unique<Texture>(iconPath);
			std::string appIconPath = Application::getAppDir() + "res\\mmw_icon.png";
			if (IO::File::exists(appIconPath)) {
				appIcon = std::make_unique<Texture>(appIconPath);
			}
			recentLoaded = true;
		}
		refreshRecentItems(recentFiles);
	
		if (!hasAutoScanned && !searchPaths.empty()) {
			scanSearchPaths();
			hasAutoScanned = true;
		}
	
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
	
		if (ImGui::Begin("GalleryScreen", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
		{
			if (openDeletePopup) { ImGui::OpenPopup(MODAL_TITLE("gallery_delete_file")); openDeletePopup = false; }
			
			if (ImGui::BeginTable("GalleryLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 200.0f);
				ImGui::TableSetupColumn("Main", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();
	
				ImGui::TableSetColumnIndex(0);
				drawSidebar();
	
				ImGui::TableSetColumnIndex(1);
				drawMainContent();
	
				ImGui::EndTable();
			}
	
			drawDeletePopup();
		}
		ImGui::End();
	}
}
