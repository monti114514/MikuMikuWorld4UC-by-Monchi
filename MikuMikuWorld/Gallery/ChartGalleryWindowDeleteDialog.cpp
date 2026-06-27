#include "ChartGalleryWindow.h"
#include "../IO.h"
#include "../IconsFontAwesome5.h"
#include "../Localization.h"
#include "../UI.h"
#include <algorithm>
#include <filesystem>

namespace MikuMikuWorld
{
	void ChartGalleryWindow::drawDeletePopup()
{
	if (ImGui::BeginPopupModal(MODAL_TITLE("gallery_delete_file"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
		std::filesystem::path p = IO::mbToWideStr(itemToDelete);
		std::string filename = "";
		std::string dirStr = "";
		std::string extStr = ""; 
		uintmax_t fileSize = 0;

		try {
			if (std::filesystem::exists(p)) {
				filename = IO::wideStringToMb(p.filename().wstring());
				dirStr = IO::wideStringToMb(p.parent_path().wstring());
				fileSize = std::filesystem::file_size(p) / 1024; 

				extStr = p.extension().string();
				if (!extStr.empty() && extStr[0] == '.') {
					extStr = extStr.substr(1); 
				}
				std::transform(extStr.begin(), extStr.end(), extStr.begin(), ::toupper);
			}
		} catch (...) {}

		ImGui::BeginGroup();
		if (appIcon) {
			ImGui::Image((void*)(intptr_t)appIcon->getID(), ImVec2(48.0f, 48.0f));
		} else {
			ImGui::SetWindowFontScale(3.0f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ICON_FA_FILE);
			ImGui::SetWindowFontScale(1.0f);
		}
		ImGui::EndGroup();

		ImGui::SameLine(0, 15.0f); 

		ImGui::BeginGroup();
		ImGui::Text("%s", getString("gallery_delete_confirm"));
		ImGui::Dummy(ImVec2(0.0f, 10.0f)); 

		ImGui::Text("%s", filename.c_str());

		if (!extStr.empty()) {
			ImGui::TextDisabled(getString("gallery_type_file"), extStr.c_str());
		} else {
			ImGui::TextDisabled("%s", getString("gallery_type_unknown"));
		}

		ImGui::TextDisabled(getString("gallery_size_kb"), fileSize);
		
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextDisabled(getString("gallery_original_location"), dirStr.c_str());
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();

		ImGui::Dummy(ImVec2(0.0f, 20.0f)); 

		float btnWidth = 100.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (btnWidth * 2 + spacing) - 10.0f);

		if (ImGui::Button(getString("gallery_yes_y"), ImVec2(btnWidth, 0))) {
			try { std::filesystem::remove(p); removeDeletedItemFromLists(itemToDelete); } catch (...) {}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(getString("gallery_no_n"), ImVec2(btnWidth, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
}
