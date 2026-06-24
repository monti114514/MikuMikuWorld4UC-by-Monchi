#include "ScoreEditorWindows.h"
#include "ApplicationConfiguration.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

#include <algorithm>

namespace MikuMikuWorld
{
	void LayersWindow::update(ScoreContext& context)
	{
		static int activeSoloIndex = -1;
		static std::vector<bool> preSoloHiddenStates;
		static bool focusRenameInput = false;

		static size_t lastLayerCount = context.score.layers.size();
		if (lastLayerCount != context.score.layers.size()) {
			activeSoloIndex = -1;
			preSoloHiddenStates.clear();
			lastLayerCount = context.score.layers.size();
		}

		bool doMergeLayer = false;
		bool doDeleteLayer = false;

		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_LAYER_GROUP, "layers")))
		{
			int toggleHideIndex = -1;
			int soloIndex = -1;
			int dragDropFrom = -1;
			int dragDropTo = -1;
			int dragDropType = -1; 

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float layersButtonHeight = ImGui::GetFrameHeight();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

			bool showAllLayers = context.showAllLayers;
			if (showAllLayers) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_PlotLinesHovered));
			if (UI::transparentButton(ICON_FA_EYE, ImVec2(layersButtonHeight, layersButtonHeight), false))
				context.showAllLayers = !context.showAllLayers;
			if (showAllLayers) ImGui::PopStyleColor();

			float rightWidth = (layersButtonHeight * 4.0f) + (4.0f * 3.0f);
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - rightWidth);

			if (UI::transparentButton(ICON_FA_PLUS, ImVec2(layersButtonHeight, layersButtonHeight), false))
			{
				Layer newLayer;
				newLayer.name = getString("new_layer");
				newLayer.isFolder = false;
				context.score.layers.push_back(newLayer);
				
				id_t id = getNextHiSpeedID();
				context.score.hiSpeedChanges[id] = { id, 0, 1, static_cast<int>(context.score.layers.size()) - 1 };
				
				renameIndex = context.score.layers.size() - 1;
				layerName = newLayer.name;
				context.selectedLayer = renameIndex;
				context.timelineEditTarget = TimelineEditTarget::Notes;
				focusRenameInput = true;
			}
			ImGui::SameLine();

			if (UI::transparentButton(ICON_FA_FOLDER_PLUS, ImVec2(layersButtonHeight, layersButtonHeight), false))
			{
				Layer newLayer;
				newLayer.name = getString("new_folder");
				newLayer.isFolder = true;
				context.score.layers.push_back(newLayer);
				
				renameIndex = context.score.layers.size() - 1;
				layerName = newLayer.name;
				context.selectedLayer = renameIndex;
				context.timelineEditTarget = TimelineEditTarget::Notes;
				focusRenameInput = true;
			}
			ImGui::SameLine();

			bool canMerge = context.selectedLayer >= 0 && context.selectedLayer < context.score.layers.size() - 1 && !context.score.layers[context.selectedLayer].isFolder;
			if (!canMerge) ImGui::BeginDisabled();
			if (UI::transparentButton(ICON_FA_LEVEL_DOWN_ALT, ImVec2(layersButtonHeight, layersButtonHeight), false))
			{
				ImGui::OpenPopup(MODAL_TITLE("layer_merge_confirm"));
			}
			if (!canMerge) ImGui::EndDisabled();
			ImGui::SameLine();

			bool canDelete = context.selectedLayer >= 0 && context.score.layers.size() > 1;
			if (!canDelete) ImGui::BeginDisabled();
			if (UI::transparentButton(ICON_FA_TRASH, ImVec2(layersButtonHeight, layersButtonHeight), false))
			{
				ImGui::OpenPopup(MODAL_TITLE("layer_delete_confirm"));
			}
			if (!canDelete) ImGui::EndDisabled();

			ImGui::PopStyleVar();
			ImGui::Separator();

			if (context.selectedLayer >= 0 && context.selectedLayer < context.score.layers.size() &&
			    !context.score.layers[context.selectedLayer].isFolder)
			{
				const int selectedLayerIndex = context.selectedLayer;
				const Layer& selectedLayer = context.score.layers[selectedLayerIndex];
				bool forceNoteSpeedEnabled = selectedLayer.forceNoteSpeed >= 1.0f &&
				                             selectedLayer.forceNoteSpeed <= 12.0f;
				float forceNoteSpeed = forceNoteSpeedEnabled ? selectedLayer.forceNoteSpeed : 6.0f;
				bool forceNoteSpeedEdited = false;

				UI::beginPropertyColumns();
				UI::propertyLabel(getString("layer_force_note_speed_enabled"));
				forceNoteSpeedEdited |= ImGui::Checkbox("##layer_force_note_speed_enabled", &forceNoteSpeedEnabled);
				ImGui::NextColumn();
				if (forceNoteSpeedEnabled)
				{
					forceNoteSpeedEdited |= UI::addFloatProperty(
					    getString("layer_force_note_speed"), forceNoteSpeed, "%.2fx", 1.0f, 12.0f);
				}
				UI::endPropertyColumns();

				if (forceNoteSpeedEdited)
				{
					if (forceNoteSpeed < 1.0f)
						forceNoteSpeed = 1.0f;
					else if (forceNoteSpeed > 12.0f)
						forceNoteSpeed = 12.0f;

					Score prev = context.score;
					context.score.layers[selectedLayerIndex].forceNoteSpeed =
					    forceNoteSpeedEnabled ? forceNoteSpeed : 0.0f;
					context.pushHistory("Change layer force note speed", prev, context.score);
				}
				ImGui::Separator();
			}
			float windowHeight =
			    ImGui::GetContentRegionAvail().y - ImGui::GetStyle().WindowPadding.y;

			if (ImGui::BeginChild("layers_child_window", ImVec2(-1, windowHeight), true))
			{
				bool hideChildren = false;

				for (int index = 0; index < context.score.layers.size(); ++index)
				{
					const auto& layer = context.score.layers[index];

					if (layer.isFolder) hideChildren = layer.isCollapsed;
					else if (!layer.inFolder) hideChildren = false;

					if (layer.inFolder && hideChildren) continue;

					ImGui::PushID(index);
					ImVec2 startPos = ImGui::GetCursorScreenPos();
					float width = ImGui::GetContentRegionAvail().x;
					bool isSelected = (index == context.selectedLayer);

					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
					}

					if (ImGui::Selectable((std::string("##row_") + std::to_string(index)).c_str(), isSelected, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(width, layersButtonHeight)))
					{
						context.selectedLayer = index;
						context.timelineEditTarget = TimelineEditTarget::Notes;
						context.selectedAudioClip = static_cast<id_t>(-1);
					}

					if (isSelected) ImGui::PopStyleColor(2);

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						renameIndex = index;
						layerName = layer.name;
						focusRenameInput = true;
					}

					int currentDropType = -1;
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						ImGui::SetDragDropPayload("LAYER_REORDER", &index, sizeof(int));
						ImGui::Text("%s %s", layer.isFolder ? ICON_FA_FOLDER : ICON_FA_FILE, layer.name.c_str());
						ImGui::EndDragDropSource();
					}
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
						{
							if (payload->IsDataType("LAYER_REORDER"))
							{
								ImVec2 min = ImGui::GetItemRectMin();
								ImVec2 max = ImGui::GetItemRectMax();
								float itemHeight = max.y - min.y;
								float mouseY = ImGui::GetMousePos().y - min.y;

								if (layer.isFolder && mouseY > itemHeight * 0.25f && mouseY < itemHeight * 0.75f) currentDropType = 1;
								else if (mouseY < itemHeight * 0.5f) currentDropType = 0;
								else currentDropType = 2;

								ImU32 redColor = IM_COL32(255, 80, 80, 255);
								if (currentDropType == 0) ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, min.y), ImVec2(max.x, min.y), redColor, 2.0f);
								else if (currentDropType == 2) ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, max.y), ImVec2(max.x, max.y), redColor, 2.0f);
								else ImGui::GetWindowDrawList()->AddRect(min, max, redColor, 0.0f, 0, 2.0f);
							}
						}
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_REORDER", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
						{
							dragDropFrom = *(const int*)payload->Data;
							dragDropTo = index;
							dragDropType = currentDropType;
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SetCursorScreenPos(ImVec2(startPos.x + ImGui::GetStyle().FramePadding.x, startPos.y));
					
					float indent = layer.inFolder ? 24.0f : 0.0f;
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

					if (layer.inFolder)
					{
						bool isLastChild = true;
						for (int j = index + 1; j < context.score.layers.size(); ++j)
						{
							if (context.score.layers[j].inFolder) { isLastChild = false; break; }
							if (context.score.layers[j].isFolder) break;
							break;
						}

						float lineX = startPos.x + 14.0f;
						float spaceY = ImGui::GetStyle().ItemSpacing.y;
						float topY = startPos.y - spaceY * 0.5f;
						float midY = startPos.y + (layersButtonHeight * 0.5f);
						float bottomY = startPos.y + layersButtonHeight + spaceY * 0.5f;

						ImU32 lineColor = IM_COL32(120, 120, 120, 255);
						ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, topY), ImVec2(lineX, isLastChild ? midY : bottomY), lineColor, 1.0f);
						ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, midY), ImVec2(lineX + 10.0f, midY), lineColor, 1.0f);
					}

					ImVec2 caretSize = ImVec2(ImGui::CalcTextSize(ICON_FA_CARET_DOWN).x + 4.0f, layersButtonHeight);
					if (layer.isFolder)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						if (ImGui::Button(layer.isCollapsed ? ICON_FA_CARET_RIGHT : ICON_FA_CARET_DOWN, caretSize))
							context.score.layers[index].isCollapsed = !layer.isCollapsed;
						ImGui::PopStyleColor();
						ImGui::SameLine();
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%s", ICON_FA_FOLDER);
						ImGui::SameLine();
					}
					else
					{
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + caretSize.x + ImGui::GetStyle().ItemSpacing.x);
					}

					float rightPanelWidth = UI::btnSmall.x * 2 + ImGui::GetStyle().ItemSpacing.x * 1 + 10.0f;

					if (renameIndex == index)
					{
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - rightPanelWidth - 10.0f);
						
						if (focusRenameInput)
						{
							ImGui::SetKeyboardFocusHere();
							focusRenameInput = false;
						}
						
						if (ImGui::InputText((std::string("##rename_") + std::to_string(index)).c_str(), &layerName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
						{
							Score prev = context.score;
							context.score.layers[index].name = layerName;
							renameIndex = -1;
							context.pushHistory("Rename Layer", prev, context.score);
						}
						else if (ImGui::IsItemDeactivated())
						{
							if (ImGui::IsKeyPressed(ImGuiKey_Escape))
							{
								renameIndex = -1;
							}
							else
							{
								Score prev = context.score;
								context.score.layers[index].name = layerName;
								renameIndex = -1;
								context.pushHistory("Rename Layer", prev, context.score);
							}
						}
					}
					else
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%s", layer.name.c_str());
					}

					ImGui::SameLine(width - rightPanelWidth);

					if (UI::transparentButton(layer.hidden ? ICON_FA_EYE_SLASH : ICON_FA_EYE, ImVec2(UI::btnSmall.x, layersButtonHeight), false))
						toggleHideIndex = index;

					ImGui::SameLine();
					if (!layer.isFolder) {
						if (activeSoloIndex == index) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_PlotLinesHovered));
						if (ImGui::Button("S", ImVec2(UI::btnSmall.x, layersButtonHeight))) soloIndex = index;
						if (activeSoloIndex == index) ImGui::PopStyleColor();
					} else {
						ImGui::Dummy(ImVec2(UI::btnSmall.x, layersButtonHeight));
					}

					ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + layersButtonHeight + ImGui::GetStyle().ItemSpacing.y));
					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			ImGui::PopStyleColor();

			if (dragDropFrom != -1 && dragDropTo != -1)
			{
				Score prev = context.score;
				
				int srcStart = dragDropFrom;
				int srcCount = 1;
				if (context.score.layers[srcStart].isFolder) {
					while (srcStart + srcCount < context.score.layers.size() && context.score.layers[srcStart + srcCount].inFolder) {
						srcCount++;
					}
				}
				
				int insertPos = dragDropTo;
				bool newInFolder = false;

				if (dragDropType == 1) {
					insertPos = dragDropTo + 1;
					newInFolder = true;
				} else if (dragDropType == 0) {
					insertPos = dragDropTo;
					newInFolder = context.score.layers[dragDropTo].inFolder;
				} else {
					insertPos = dragDropTo + 1;
					newInFolder = context.score.layers[dragDropTo].inFolder;
				}

				if (!(insertPos >= srcStart && insertPos <= srcStart + srcCount))
				{
					if (!context.score.layers[srcStart].isFolder) {
						context.score.layers[srcStart].inFolder = newInFolder;
					}

					std::vector<int> oldIndices(context.score.layers.size());
					for (int i = 0; i < oldIndices.size(); ++i) oldIndices[i] = i;
					
					std::vector<int> oldBlock(oldIndices.begin() + srcStart, oldIndices.begin() + srcStart + srcCount);
					oldIndices.erase(oldIndices.begin() + srcStart, oldIndices.begin() + srcStart + srcCount);
					if (insertPos > srcStart) insertPos -= srcCount;
					oldIndices.insert(oldIndices.begin() + insertPos, oldBlock.begin(), oldBlock.end());
					
					std::unordered_map<int, int> oldToNew;
					for (int newIdx = 0; newIdx < oldIndices.size(); ++newIdx) oldToNew[oldIndices[newIdx]] = newIdx;

					std::vector<Layer> newLayers(context.score.layers.size());
					for (int i = 0; i < oldIndices.size(); ++i) {
						newLayers[i] = context.score.layers[oldIndices[i]];
					}
					context.score.layers = newLayers;

					for (auto& [_, note] : context.score.notes) note.layer = oldToNew[note.layer];
					for (auto& [_, hiSpeed] : context.score.hiSpeedChanges) hiSpeed.layer = oldToNew[hiSpeed.layer];
					context.selectedLayer = oldToNew[context.selectedLayer];

					context.pushHistory("Reorder Layers/Folders", prev, context.score);
				}
			}

			if (toggleHideIndex != -1)
			{
				Score prev = context.score;
				auto& layer = context.score.layers[toggleHideIndex];
				layer.hidden = !layer.hidden;
				
				if (layer.isFolder) {
					for (int i = toggleHideIndex + 1; i < context.score.layers.size(); ++i) {
						if (!context.score.layers[i].inFolder) break;
						context.score.layers[i].hidden = layer.hidden;
					}
				}
				context.pushHistory("Toggle Hide Layer", prev, context.score);
			}

			if (soloIndex != -1)
			{
				Score prev = context.score;
				if (activeSoloIndex == soloIndex)
				{
					for (int i = 0; i < context.score.layers.size() && i < preSoloHiddenStates.size(); ++i)
					{
						context.score.layers[i].hidden = preSoloHiddenStates[i];
					}
					activeSoloIndex = -1;
				}
				else
				{
					if (activeSoloIndex == -1)
					{
						preSoloHiddenStates.clear();
						for (const auto& l : context.score.layers) preSoloHiddenStates.push_back(l.hidden);
					}
					for (int i = 0; i < context.score.layers.size(); ++i)
					{
						context.score.layers[i].hidden = (i != soloIndex);
					}
					activeSoloIndex = soloIndex;
				}
				context.pushHistory("Toggle Solo Layer", prev, context.score);
			}

			if (ImGui::BeginPopupModal(MODAL_TITLE("layer_delete_confirm"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::Text("%s", getString("layer_delete_msg1"));
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", getString("layer_delete_msg2"));
				ImGui::Separator();
				
				if (ImGui::Button(getString("yes"), ImVec2(120, 0))) {
					doDeleteLayer = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button(getString("cancel"), ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal(MODAL_TITLE("layer_merge_confirm"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::Text("%s", getString("layer_merge_msg"));
				ImGui::Separator();
				
				if (ImGui::Button(getString("yes"), ImVec2(120, 0))) {
					doMergeLayer = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button(getString("cancel"), ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();

		if (doMergeLayer)
		{
			int mergePattern = context.selectedLayer;
			Score prev = context.score;
			context.score.layers.erase(context.score.layers.begin() + mergePattern);
			for (auto& [_, note] : context.score.notes)
			{
				if (note.layer > mergePattern) note.layer -= 1;
			}
			for (auto& [_, hiSpeed] : context.score.hiSpeedChanges)
			{
				if (hiSpeed.layer > mergePattern) hiSpeed.layer -= 1;
			}
			context.pushHistory("Merge Layer", prev, context.score);
		}

		if (doDeleteLayer)
		{
			int deleteIndex = context.selectedLayer;
			int delCount = 1;
			if (context.score.layers[deleteIndex].isFolder) {
				while (deleteIndex + delCount < context.score.layers.size() && context.score.layers[deleteIndex + delCount].inFolder) {
					delCount++;
				}
			}

			if (context.score.layers.size() - delCount >= 1)
			{
				Score prev = context.score;

				std::unordered_set<int> notesToDelete;
				for (const auto& [id, note] : context.score.notes) {
					if (note.layer >= deleteIndex && note.layer < deleteIndex + delCount) notesToDelete.insert(id);
				}

				for (auto id : notesToDelete) {
					auto notePos = context.score.notes.find(id);
					if (notePos == context.score.notes.end()) continue;

					Note& note = notePos->second;
					if (note.getType() != NoteType::Hold && note.getType() != NoteType::HoldEnd) {
						if (note.getType() == NoteType::HoldMid && context.score.holdNotes.count(note.parentID)) {
							std::vector<HoldStep>& steps = context.score.holdNotes.at(note.parentID).steps;
							auto stepIt = std::find_if(steps.cbegin(), steps.cend(), [id](const HoldStep& s) { return s.ID == id; });
							if (stepIt != steps.cend()) steps.erase(stepIt);
						}
						context.score.notes.erase(id);
					} else {
						const HoldNote& hold = context.score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
						context.score.notes.erase(hold.start.ID);
						context.score.notes.erase(hold.end);
						for (const auto& step : hold.steps) context.score.notes.erase(step.ID);
						context.score.holdNotes.erase(hold.start.ID);
					}
				}

				std::unordered_set<int> hiSpeedsToDelete;
				for (const auto& [id, hs] : context.score.hiSpeedChanges) {
					if (hs.layer >= deleteIndex && hs.layer < deleteIndex + delCount) hiSpeedsToDelete.insert(id);
				}
				for (auto id : hiSpeedsToDelete) context.score.hiSpeedChanges.erase(id);

				for (auto it = context.selectedNotes.begin(); it != context.selectedNotes.end(); ) {
					if (!context.score.notes.count(*it)) it = context.selectedNotes.erase(it);
					else ++it;
				}
				for (auto it = context.selectedHiSpeedChanges.begin(); it != context.selectedHiSpeedChanges.end(); ) {
					if (!context.score.hiSpeedChanges.count(*it)) it = context.selectedHiSpeedChanges.erase(it);
					else ++it;
				}

				context.score.layers.erase(context.score.layers.begin() + deleteIndex, context.score.layers.begin() + deleteIndex + delCount);
				for (auto& [_, note] : context.score.notes) if (note.layer >= deleteIndex + delCount) note.layer -= delCount;
				for (auto& [_, hiSpeed] : context.score.hiSpeedChanges) if (hiSpeed.layer >= deleteIndex + delCount) hiSpeed.layer -= delCount;
				
				if (context.selectedLayer >= deleteIndex && context.selectedLayer < deleteIndex + delCount) context.selectedLayer = 0;
				else if (context.selectedLayer >= deleteIndex + delCount) context.selectedLayer -= delCount;

				context.pushHistory("Delete Layer/Folder", prev, context.score);
			}
		}
	}

	DialogResult LayersWindow::updateCreationDialog()
	{
		DialogResult result = DialogResult::None;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		
		bool popupOpened = false;
		if (renameIndex == -2)
			popupOpened = ImGui::BeginPopupModal(MODAL_TITLE("create_folder"), NULL, ImGuiWindowFlags_NoResize);
		else if (renameIndex >= 0)
			popupOpened = ImGui::BeginPopupModal(MODAL_TITLE("layer_rename"), NULL, ImGuiWindowFlags_NoResize);
		else
			popupOpened = ImGui::BeginPopupModal(MODAL_TITLE("create_layer"), NULL, ImGuiWindowFlags_NoResize);

		if (popupOpened)
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnSmall.y - 2.0f - (padding.y * 2);

			if (renameIndex == -2)
				ImGui::Text("%s", getString("layer_name"));
			else
				ImGui::Text("%s", getString("layer_name"));

			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##layer_name", &layerName);

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
			                  2.0f,
			              ImGui::GetFrameHeight() };
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				result = DialogResult::Cancel;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !layerName.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !layerName.size()));
			
			if (ImGui::Button(getString("confirm"), btnSz))
			{
				result = DialogResult::Ok;
				ImGui::CloseCurrentPopup();
			}

			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}

		return result;
	}
}
