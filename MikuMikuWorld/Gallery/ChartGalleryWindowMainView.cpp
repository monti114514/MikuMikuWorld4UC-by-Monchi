#include "ChartGalleryWindow.h"
#include "../IconsFontAwesome5.h"
#include "../Localization.h"
#include <algorithm>
#include <cstring>
namespace
{
	std::string toLowerAscii(std::string s)
	{
		for (char& c : s) {
			if (c >= 'A' && c <= 'Z')
				c += ('a' - 'A');
		}
		return s;
	}
}

namespace MikuMikuWorld
{
void ChartGalleryWindow::drawMainContent()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 0.0f));

	if (ImGui::BeginChild("MainContentChild", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
		pollFolderDialog();

		std::string filterFolder = "";
		if (activeTab == 3) filterFolder = "Team Projects";
		else if (activeTab == 4) filterFolder = "Personal";
		else if (activeTab >= 5 && (activeTab - 5) < (int)customFolders.size()) filterFolder = customFolders[activeTab - 5];

		if (activeTab == 0 || activeTab == 1) {
			std::vector<std::shared_ptr<GalleryItem>> recentToDraw = recentItems;
			bool hasMore = (activeTab == 0 && recentToDraw.size() > 4);
			if (hasMore) recentToDraw.resize(4);

			ImGui::Spacing(); 
			ImGui::Text("%s", getString("gallery_recent_charts"));
			ImGui::Separator();
			ImGui::Spacing();

			drawGrid(recentToDraw, "RecentGrid");

			if (hasMore) {
				ImGui::SameLine(0, 16.0f);
				ImGui::BeginGroup();
				float moreWidth = 100.0f;
				float moreHeight = 165.0f;
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddRectFilled(pos, ImVec2(pos.x + moreWidth, pos.y + moreHeight), IM_COL32(50, 50, 50, 255), 5.0f);
				if (ImGui::InvisibleButton("ShowMoreBtn", ImVec2(moreWidth, moreHeight))) activeTab = 1;
				if (ImGui::IsItemHovered()) dl->AddRectFilled(pos, ImVec2(pos.x + moreWidth, pos.y + moreHeight), IM_COL32(255, 255, 255, 20), 5.0f);
				
				const char* iconTxt = ICON_FA_ARROW_RIGHT;
				const char* moreTxt = getString("gallery_more");
				ImVec2 iconSize = ImGui::CalcTextSize(iconTxt);
				ImVec2 textSize = ImGui::CalcTextSize(moreTxt);
				float centerX = pos.x + (moreWidth / 2.0f);
				dl->AddText(ImVec2(centerX - iconSize.x / 2.0f, pos.y + (moreHeight / 2.0f) - 15), IM_COL32(200, 200, 200, 255), iconTxt);
				dl->AddText(ImVec2(centerX - textSize.x / 2.0f, pos.y + (moreHeight / 2.0f) + 5), IM_COL32(200, 200, 200, 255), moreTxt);
				ImGui::EndGroup();
			}
		}
		
		if (activeTab == 0)
		{
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Text("%s", getString("gallery_all_charts"));
			ImGui::Separator();
			ImGui::Spacing();

			std::string toggleBtnText = isSearchPathsOpen ? (std::string(ICON_FA_CHEVRON_DOWN) + " " + getString("gallery_hide_search_paths")) : (std::string(ICON_FA_CHEVRON_RIGHT) + " " + getString("gallery_show_search_paths"));
			if (ImGui::Button(toggleBtnText.c_str()))
			{
				isSearchPathsOpen = !isSearchPathsOpen;
				saveGalleryData();
			}

			ImGui::SameLine();

			ImVec4 accentColor = ImGui::GetStyle().Colors[ImGuiCol_SliderGrab];
			ImVec4 accentHovered = accentColor; accentHovered.w = 0.8f; 
			ImVec4 accentActive = ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive]; 

			ImGui::PushStyleColor(ImGuiCol_Button, accentColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentHovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentActive);

			if (ImGui::Button(getString("gallery_rescan_paths")))
			{
				saveGalleryData();
				scanSearchPaths();
			}
			ImGui::PopStyleColor(3); 

			ImGui::SameLine();

			float sortDirectionWidth = 110.0f;
			float totalWidth = 250.0f + 170.0f + 150.0f + sortDirectionWidth + 42.0f; 
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - 16.0f); 

			ImGui::TextDisabled(ICON_FA_SEARCH); 
			float iconWidth = ImGui::CalcTextSize(ICON_FA_SEARCH).x;
			float searchBoxWidth = 250.0f;

			ImGui::SetNextItemWidth(searchBoxWidth - iconWidth - 8.0f);
			ImGui::SameLine(); 
			ImGui::InputText("##SearchBox", searchQuery, sizeof(searchQuery));

			ImGui::SameLine(); 

			const char* sortModeOptions[] = {
				getString("gallery_modified"),
				getString("gallery_created"),
				getString("gallery_name_sort"),
				getString("gallery_combo")
			};
			ImGui::SetNextItemWidth(170.0f);
			ImGui::Combo("##SortModeBox", &sortMode, sortModeOptions, 4);

			ImGui::SameLine();

			const char* nameSortTargetOptions[] = {
				getString("gallery_title"),
				getString("gallery_file"),
				getString("gallery_artist"),
				getString("gallery_author")
			};
			ImGui::BeginDisabled(sortMode != 2);
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.ItemSpacing.y + 4.0f));
			ImGui::SetNextItemWidth(150.0f);
			ImGui::Combo("##NameSortTargetBox", &nameSortTarget, nameSortTargetOptions, 4);
			ImGui::PopStyleVar();
			ImGui::EndDisabled();

			ImGui::SameLine();
			std::string sortDirectionLabel = std::string(sortAscending ? ICON_FA_SORT_AMOUNT_UP : ICON_FA_SORT_AMOUNT_DOWN) + " " + getString(sortAscending ? "sort_ascending" : "sort_descending");
			if (ImGui::Button(sortDirectionLabel.c_str(), ImVec2(sortDirectionWidth, 0.0f))) {
				sortAscending = !sortAscending;
			}
			ImGui::Spacing(); 

			if (isSearchPathsOpen) {
				ImGui::Dummy(ImVec2(0.0f, 4.0f));
				ImGui::Indent();

				int pathToDeleteIndex = -1;

				for (int pIdx = 0; pIdx < (int)searchPaths.size(); ++pIdx) {
					ImGui::PushID(pIdx);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					
					if (ImGui::Button(ICON_FA_TRASH)) {
						pathToDeleteIndex = pIdx; 
					}
					
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::Text("%s", searchPaths[pIdx].c_str()); 
					ImGui::PopID();
				}

				if (pathToDeleteIndex != -1) {
					searchPaths.erase(searchPaths.begin() + pathToDeleteIndex);
					pathInputError.clear();
					saveGalleryData();
					scanSearchPaths();
				}

				ImGui::Dummy(ImVec2(0.0f, 4.0f));

				ImGui::Text("%s", getString("gallery_add_path"));
				ImGui::SetNextItemWidth(400.0f);
				bool addTypedPath = ImGui::InputTextWithHint("##NewPath", getString("gallery_path_input_hint"), newPathBuffer, sizeof(newPathBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SameLine();
				ImGui::BeginDisabled(folderDialogInProgress);
				if (ImGui::Button((std::string(ICON_FA_FOLDER_OPEN) + " " + getString("gallery_browse_folder")).c_str()))
					startFolderDialog();
				ImGui::EndDisabled();

				if (folderDialogInProgress) {
					ImGui::SameLine();
					ImGui::TextDisabled("%s", getString("gallery_folder_dialog_open"));
				}

				if (addTypedPath)
				{
					if (addSearchPath(newPathBuffer))
						memset(newPathBuffer, 0, sizeof(newPathBuffer));
				}
				if (!pathInputError.empty()) {
					ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "%s", pathInputError.c_str());
				}
				ImGui::Unindent();
				ImGui::Dummy(ImVec2(0.0f, 8.0f));
			}

			std::vector<std::shared_ptr<GalleryItem>> displayItems;
			std::string query = toLowerAscii(searchQuery);

			for (const auto& item : localItems) {
				if (query.empty()) {
					displayItems.push_back(item);
					continue;
				}
				
				std::string titleLower = toLowerAscii(item->title);
				std::string artistLower = toLowerAscii(item->artist);
				std::string filenameLower = toLowerAscii(item->filename);

				if (titleLower.find(query) != std::string::npos ||
					artistLower.find(query) != std::string::npos ||
					filenameLower.find(query) != std::string::npos) {
					displayItems.push_back(item);
				}
			}

			sortItems(displayItems);

			drawGrid(displayItems, "LocalGrid");
		}

		if (activeTab == 2) {
			std::vector<std::shared_ptr<GalleryItem>> favs;
			for (auto& it : recentItems) if (it->isFavorite) favs.push_back(it);
			for (auto& it : localItems) if (it->isFavorite) {
				if (std::find_if(favs.begin(), favs.end(), [&](auto& f){return f->filepath == it->filepath;}) == favs.end()) favs.push_back(it);
			}
			ImGui::Spacing(); ImGui::Text("%s", getString("gallery_favorites")); ImGui::Separator(); ImGui::Spacing();
			drawGrid(favs, "FavoritesGrid");
		}

		if (activeTab >= 3) {
			std::vector<std::shared_ptr<GalleryItem>> fItems;
			for (auto& it : recentItems) if (it->folder == filterFolder) fItems.push_back(it);
			for (auto& it : localItems) if (it->folder == filterFolder) {
				if (std::find_if(fItems.begin(), fItems.end(), [&](auto& f){return f->filepath == it->filepath;}) == fItems.end()) fItems.push_back(it);
			}
			
			std::string displayFilter = filterFolder;
			if (filterFolder == "Team Projects") displayFilter = getString("gallery_team_projects");
			else if (filterFolder == "Personal") displayFilter = getString("gallery_personal");
			
			ImGui::Spacing(); ImGui::Text("%s", displayFilter.c_str()); ImGui::Separator(); ImGui::Spacing();
			drawGrid(fItems, "CustomGrid");
		}
		ImGui::Dummy(ImVec2(0.0f, 24.0f));
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}
}
