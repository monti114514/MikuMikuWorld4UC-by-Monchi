#include "ChartGalleryWindow.h"
#include "../IconsFontAwesome5.h"
#include "../Localization.h"
#include <cstring>

namespace MikuMikuWorld
{
	void ChartGalleryWindow::drawSidebar()
	{
		if (ImGui::BeginChild("SidebarChild", ImVec2(0, 0), false)) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
	
		ImGui::Dummy(ImVec2(0.0f, 12.0f));

		auto drawLargeSidebarButton = [&](const char* id, const char* text, Texture* iconTex, const char* leftIconStr, const char* rightIconStr) -> bool {
			ImVec2 btnPos = ImGui::GetCursorScreenPos();
			float btnWidth = ImGui::GetContentRegionAvail().x - 8.0f;
			float btnHeight = 60.0f;

			ImGui::InvisibleButton(id, ImVec2(btnWidth, btnHeight));
			bool clicked = ImGui::IsItemClicked();

			bool hovered = ImGui::IsItemHovered();
			bool active = ImGui::IsItemActive();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImU32 bgColor = active ? IM_COL32(50, 50, 50, 255) : (hovered ? IM_COL32(85, 85, 85, 255) : IM_COL32(65, 65, 65, 255));
			drawList->AddRectFilled(btnPos, ImVec2(btnPos.x + btnWidth, btnPos.y + btnHeight), bgColor, 4.0f);

			float iconSizeVal = 32.0f;
			float textStartX = btnPos.x + 12.0f;

			if (iconTex) {
				ImVec2 iconPos = ImVec2(btnPos.x + 10.0f, btnPos.y + (btnHeight - iconSizeVal) * 0.5f);
				drawList->AddImage((void*)(intptr_t)iconTex->getID(), iconPos, ImVec2(iconPos.x + iconSizeVal, iconPos.y + iconSizeVal));
				textStartX = iconPos.x + iconSizeVal + 12.0f;
			} else if (leftIconStr) {
				ImVec2 leftIconSize = ImGui::CalcTextSize(leftIconStr);
				ImVec2 iconPos = ImVec2(btnPos.x + 16.0f, btnPos.y + (btnHeight - leftIconSize.y) * 0.5f);
				drawList->AddText(iconPos, IM_COL32(200, 200, 200, 255), leftIconStr);
				textStartX = iconPos.x + leftIconSize.x + 16.0f;
			}

			ImVec2 textSize = ImGui::CalcTextSize(text);
			float textY = btnPos.y + (btnHeight - textSize.y) * 0.5f;
			drawList->AddText(ImVec2(textStartX, textY), IM_COL32(240, 240, 240, 255), text);

			if (rightIconStr) {
				ImVec2 rightIconSize = ImGui::CalcTextSize(rightIconStr);
				float rightIconX = btnPos.x + btnWidth - rightIconSize.x - 12.0f;
				float rightIconY = btnPos.y + (btnHeight - rightIconSize.y) * 0.5f;
				drawList->AddText(ImVec2(rightIconX, rightIconY), IM_COL32(180, 180, 180, 255), rightIconStr);
			}

			return clicked;
		};

		if (drawLargeSidebarButton("CreateNewChartBtn", getString("gallery_create_new_chart"), appIcon.get(), nullptr, ICON_FA_PLUS)) {
			pendingCreateNew = true;
			open = false;
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		if (drawLargeSidebarButton("ReturnToEditorBtn", getString("gallery_return_to_editor"), nullptr, ICON_FA_ARROW_LEFT, nullptr)) {
			open = false;
		}

			ImGui::Dummy(ImVec2(0.0f, 12.0f));
	
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); 
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
	
			if (ImGui::Button(ICON_FA_PLUS, ImVec2(30, 30))) {
				isCreatingNewFolder = true;
				memset(folderEditBuffer, 0, sizeof(folderEditBuffer));
			}
			ImGui::SameLine();
	
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(30, 30))) { if (activeTab >= 5) deleteFolder(activeTab); }
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_PEN, ImVec2(30, 30))) {
				if (activeTab >= 5) {
					editingFolderIndex = activeTab;
					std::string currentName = customFolders[activeTab - 5];
					strncpy(folderEditBuffer, currentName.c_str(), 256);
				}
			}
	
			ImGui::PopStyleColor(3);
	
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 4.0f));
	
			auto drawSelectable = [&](const char* label, int index) {
				bool selected = (activeTab == index);
				if (editingFolderIndex == index) {
					ImGui::SetNextItemWidth(-8);
					ImGui::SetKeyboardFocusHere();
					if (ImGui::InputText("##EditFolder", folderEditBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
						std::string oldName = customFolders[index - 5];
						std::string newName = folderEditBuffer;
						if (!newName.empty()) {
							customFolders[index - 5] = newName;
							for (auto& [path, state] : galleryStates) if (state.folder == oldName) state.folder = newName;
							for (auto& item : recentItems) if (item->folder == oldName) item->folder = newName;
							for (auto& item : localItems) if (item->folder == oldName) item->folder = newName;
							saveGalleryData();
						}
						editingFolderIndex = -1;
					}
					if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit()) editingFolderIndex = -1;
				} else {
					ImVec2 pos = ImGui::GetCursorScreenPos();
					float width = ImGui::GetContentRegionAvail().x - 8.0f;
					float height = 30.0f;
	
					ImGui::InvisibleButton(label, ImVec2(width, height));
					if (ImGui::IsItemClicked()) activeTab = index;
					
					bool hovered = ImGui::IsItemHovered();
					
					if (hovered && ImGui::IsMouseDoubleClicked(0) && index >= 5) {
						editingFolderIndex = index;
						std::string currentName = customFolders[index - 5];
						strncpy(folderEditBuffer, currentName.c_str(), 256);
					}
	
					ImDrawList* drawList = ImGui::GetWindowDrawList();
	
					ImU32 bgColor;
					if (selected) {
						bgColor = ImGui::GetColorU32(ImGuiCol_SliderGrab);
					} else if (hovered) {
						bgColor = IM_COL32(85, 85, 85, 255);   
					} else {
						bgColor = IM_COL32(65, 65, 65, 255);   
					}
	
					drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgColor, 3.0f);
	
					ImVec2 textSize = ImGui::CalcTextSize(label);
					float textY = pos.y + (height - textSize.y) * 0.5f; 
	
					float arrowIconStartX = pos.x + width - 24.0f; 
					ImVec2 textClipMin = ImVec2(pos.x + 12.0f, pos.y);
					ImVec2 textClipMax = ImVec2(arrowIconStartX - 8.0f, pos.y + height);
	
					drawList->PushClipRect(textClipMin, textClipMax, true);
					drawList->AddText(ImVec2(pos.x + 12.0f, textY), IM_COL32(230, 230, 230, 255), label);
					drawList->PopClipRect();
	
					const char* arrowIcon = ICON_FA_CHEVRON_RIGHT;
					ImVec2 arrowSize = ImGui::CalcTextSize(arrowIcon);
					float arrowX = pos.x + width - arrowSize.x - 12.0f;
					float arrowY = pos.y + (height - arrowSize.y) * 0.5f;
					drawList->AddText(ImVec2(arrowX, arrowY), IM_COL32(180, 180, 180, 255), arrowIcon);
	
					ImGui::Dummy(ImVec2(0.0f, 4.0f)); 
				}
			};
	
			drawSelectable((std::string("  ") + getString("gallery_all")).c_str(), 0);
			drawSelectable((std::string("  ") + getString("gallery_recent")).c_str(), 1);
			drawSelectable((std::string("  ") + getString("gallery_favorites")).c_str(), 2);
			drawSelectable((std::string("  ") + getString("gallery_team_projects")).c_str(), 3);
			drawSelectable((std::string("  ") + getString("gallery_personal")).c_str(), 4);
	
			ImGui::Separator(); 
			ImGui::Dummy(ImVec2(0.0f, 4.0f));
	
			for (int i = 0; i < (int)customFolders.size(); ++i) {
				drawSelectable((std::string("  ") + customFolders[i]).c_str(), 5 + i);
			}
	
			if (isCreatingNewFolder) {
				ImGui::SetNextItemWidth(-8);
				ImGui::SetKeyboardFocusHere();
				if (ImGui::InputText("##NewFolder", folderEditBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
					if (strlen(folderEditBuffer) > 0) { customFolders.push_back(folderEditBuffer); saveGalleryData(); }
					isCreatingNewFolder = false;
				}
				if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit()) isCreatingNewFolder = false;
			}
		}
		ImGui::EndChild();
	}
}
