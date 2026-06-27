#include "ChartGalleryWindow.h"
#include "../IconsFontAwesome5.h"
#include "../Localization.h"
#include <algorithm>

namespace MikuMikuWorld
{
	void ChartGalleryWindow::drawGrid(std::vector<std::shared_ptr<GalleryItem>>& itemsToDraw, const char* gridId)
	{
		if (itemsToDraw.empty()) {
			ImGui::TextDisabled("%s", (std::string("  ") + getString("gallery_no_charts")).c_str());
			return;
		}

		float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		float cardWidth = 315.0f; 
		float cardHeight = 165.0f; 
		float spacing = 24.0f;
		float padding = 8.0f;
		float imageSize = 120.0f;

		for (int i = 0; i < (int)itemsToDraw.size(); ++i)
		{
			auto& item = itemsToDraw[i];
			if (galleryStates.count(item->filepath)) {
				item->isFavorite = galleryStates[item->filepath].isFavorite;
				item->folder = galleryStates[item->filepath].folder;
			}

			ImGui::PushID(gridId); 
			ImGui::PushID(item->filepath.c_str());

			ImGui::BeginGroup(); 
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(cardWidth, cardHeight));
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + cardWidth, cursorPos.y + cardHeight), IM_COL32(35, 35, 35, 255), 5.0f);

			ImVec2 imgPos = ImVec2(cursorPos.x + padding, cursorPos.y + padding);
			if (item->texture) {

				ImVec2 uv0 = ImVec2(0.0f, 0.0f);
				ImVec2 uv1 = ImVec2(1.0f, 1.0f);
				float texW = (float)item->texture->getWidth();
				float texH = (float)item->texture->getHeight();
				
				if (texW > texH) {

					float offset = (texW - texH) / 2.0f;
					uv0.x = offset / texW;
					uv1.x = (offset + texH) / texW;
				} else if (texH > texW) {

					float offset = (texH - texW) / 2.0f;
					uv0.y = offset / texH;
					uv1.y = (offset + texW) / texH;
				}
				

				drawList->AddImageRounded((void*)(intptr_t)item->texture->getID(), imgPos, ImVec2(imgPos.x + imageSize, imgPos.y + imageSize), uv0, uv1, IM_COL32(255, 255, 255, 255), 4.0f);

			} else if (defaultIcon) {
				drawList->AddImageRounded((void*)(intptr_t)defaultIcon->getID(), imgPos, ImVec2(imgPos.x + imageSize, imgPos.y + imageSize), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 4.0f);
			}

			ImVec2 heartBtnPos = ImVec2(imgPos.x + 4, imgPos.y + 4);
			drawList->AddRectFilled(heartBtnPos, ImVec2(heartBtnPos.x + 24, heartBtnPos.y + 24), IM_COL32(0, 0, 0, 120), 4.0f);
			
			ImGui::SetCursorScreenPos(heartBtnPos);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, item->isFavorite ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 0.7f));
			if (ImGui::Button(ICON_FA_HEART, ImVec2(24, 24))) {
				item->isFavorite = !item->isFavorite;
				galleryStates[item->filepath].isFavorite = item->isFavorite;
				saveGalleryData();
			}
			bool favHovered = ImGui::IsItemHovered();
			ImGui::PopStyleColor(2);

			float textStartX = imgPos.x + imageSize + padding + 15.0f;
			float textStartY = cursorPos.y + padding + 2.0f;
			float labelWidth = 65.0f;
			float valueStartX = textStartX + labelWidth + 12.0f;
			drawList->AddLine(ImVec2(textStartX + labelWidth, textStartY), ImVec2(textStartX + labelWidth, textStartY + 7 * 16.5f), IM_COL32(80, 80, 80, 255));

			const char* labels[] = { getString("gallery_file"), getString("gallery_format"), getString("gallery_title"), getString("gallery_artist"), getString("gallery_author"), getString("gallery_time"), getString("gallery_combo") };
			std::string values[] = { item->filename, item->extension, item->title, item->artist, item->author, item->lengthStr, std::to_string(item->totalCombo) };
			for (int row = 0; row < 7; ++row) {
				float rowY = textStartY + row * 16.5f;
				drawList->AddText(ImVec2(textStartX, rowY), IM_COL32(160, 160, 160, 255), labels[row]);
				ImVec2 clipMin(valueStartX, rowY);
				ImVec2 clipMax(cursorPos.x + cardWidth - padding, rowY + 16.0f);
				drawList->PushClipRect(clipMin, clipMax, true);
				drawList->AddText(clipMin, IM_COL32(230, 230, 230, 255), values[row].c_str());
				drawList->PopClipRect();
			}

			float bottomY = imgPos.y + imageSize + 7.5f;
			ImVec2 titleBarStart(imgPos.x, bottomY);
			ImVec2 titleBarEnd(imgPos.x + imageSize, bottomY + 22.0f);
			drawList->AddRectFilled(titleBarStart, titleBarEnd, IM_COL32(45, 45, 45, 255));

			ImVec2 textSize = ImGui::CalcTextSize(item->title.c_str());
			float textX = titleBarStart.x + (imageSize - textSize.x) * 0.5f;
			if (textX < titleBarStart.x + 4.0f) textX = titleBarStart.x + 4.0f;

			drawList->PushClipRect(titleBarStart, titleBarEnd, true);
			drawList->AddText(ImVec2(textX, titleBarStart.y + 4.0f), IM_COL32(255, 255, 255, 255), item->title.c_str());
			drawList->PopClipRect();
			

			std::string displayFolderName = item->folder;
			if (displayFolderName == "Team Projects") displayFolderName = getString("gallery_team_projects");
			else if (displayFolderName == "Personal") displayFolderName = getString("gallery_personal");
			else if (displayFolderName == "-") displayFolderName = getString("gallery_uncategorized");
			
			std::string tagText = std::string(ICON_FA_TAG " ") + displayFolderName;
			ImVec2 tagPos = ImVec2(imgPos.x + imageSize + 12.0f, bottomY + 4.0f);

			float trashButtonStartX = cursorPos.x + cardWidth - padding - 24.0f;
			ImVec2 clipMin = tagPos;
			ImVec2 clipMax = ImVec2(trashButtonStartX - 8.0f, tagPos.y + 20.0f);

			drawList->PushClipRect(clipMin, clipMax, true);
			drawList->AddText(tagPos, IM_COL32(153, 153, 153, 255), tagText.c_str());
			drawList->PopClipRect();

			ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + cardWidth - padding - 24.0f, bottomY - 2.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(24, 24))) { itemToDelete = item->filepath; openDeletePopup = true; }
			bool trashHovered = ImGui::IsItemHovered();
			ImGui::PopStyleColor();

			ImGui::EndGroup();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 6.0f));

			if (ImGui::BeginPopupContextItem("ChartContextMenu"))
			{
					if (ImGui::BeginMenu(getString("gallery_move_to_folder")))
					{
					if (ImGui::MenuItem(getString("gallery_uncategorized"), NULL, item->folder == "-")) {
						item->folder = "-";
						galleryStates[item->filepath].folder = "-";
						saveGalleryData();
					}
					
					ImGui::Dummy(ImVec2(0.0f, 2.0f));
					ImGui::Separator();
					ImGui::Dummy(ImVec2(0.0f, 2.0f));

					std::vector<std::string> displayFolders = { "Team Projects", "Personal" };
					for (const auto& f : customFolders) {
						if (f != "Team Projects" && f != "Personal") displayFolders.push_back(f);
					}

					for (const auto& folderName : displayFolders) {

						std::string menuDisplayName = folderName;
						if (folderName == "Team Projects") menuDisplayName = getString("gallery_team_projects");
						else if (folderName == "Personal") menuDisplayName = getString("gallery_personal");
						
						if (ImGui::MenuItem(menuDisplayName.c_str(), NULL, item->folder == folderName)) {
							item->folder = folderName;
							galleryStates[item->filepath].folder = folderName;
							saveGalleryData();
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar(3); 

			if (ImGui::IsItemHovered())
				drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + cardWidth, cursorPos.y + cardHeight), IM_COL32(255, 255, 255, 15), 5.0f);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !favHovered && !trashHovered) {
				pendingLoadScore = item->filepath;
			}

			float lastGroupX = ImGui::GetItemRectMax().x;
			float nextGroupX = lastGroupX + spacing + cardWidth;
			if (i + 1 < (int)itemsToDraw.size()) {
				if (nextGroupX < windowVisibleX) ImGui::SameLine(0, spacing);
				else ImGui::Dummy(ImVec2(0, 10.0f));
			}

		ImGui::PopID();
		ImGui::PopID();
		}
	}
}
