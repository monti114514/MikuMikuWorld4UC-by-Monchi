#include "ScoreEditorWindows.h"
#include "ApplicationConfiguration.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

#include <algorithm>

namespace MikuMikuWorld
{
	void WaypointsWindow::update(ScoreContext& context)
	{
		static int renameIndex = -1;
		static std::string waypointName = "";
		static bool focusRenameInput = false;
		static int selectedWaypointIndex = -1;
		static bool descendingOrder = true;
		static int gotoMeasure = 0;

		auto getContrastColor = [](const ImVec4& bg) -> ImVec4 {
			float luminance = bg.x * 0.299f + bg.y * 0.587f + bg.z * 0.114f;
			return luminance > 0.5f ? ImVec4(0.1f, 0.1f, 0.1f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		};

		ImVec4 activeBgColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		ImVec4 activeTextColor = getContrastColor(activeBgColor);

		auto normalizeWaypointSelection = [&]()
		{
			const int waypointCount = static_cast<int>(context.score.waypoints.size());
			if (waypointCount <= 0)
			{
				selectedWaypointIndex = -1;
				renameIndex = -1;
				return;
			}

			if (selectedWaypointIndex >= waypointCount)
				selectedWaypointIndex = waypointCount - 1;
			if (selectedWaypointIndex < -1)
				selectedWaypointIndex = -1;
			if (renameIndex >= waypointCount)
				renameIndex = -1;
		};

		normalizeWaypointSelection();

		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_LOCATION_ARROW, "waypoints")))
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float waypointButtonHeight = ImGui::GetFrameHeight();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

			ImGui::SetNextItemWidth(58.0f);
			bool gotoActivated = false;
			ImGui::InputInt("##waypoint_goto_measure", &gotoMeasure, 0, 0,
			                ImGuiInputTextFlags_AutoSelectAll);
			gotoActivated |= ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false);

			ImGui::SameLine();
			gotoActivated |= UI::transparentButton(ICON_FA_ARROW_RIGHT,
			                                       ImVec2(waypointButtonHeight, waypointButtonHeight),
			                                       false);
			if (gotoActivated)
			{
				gotoMeasure = std::max(gotoMeasure, 0);
				scrollTimeline(
				    context, measureToTicks(gotoMeasure, TICKS_PER_BEAT, context.score.timeSignatures));
			}

			std::string sortDirectionLabel =
			    std::string(descendingOrder ? ICON_FA_SORT_AMOUNT_DOWN : ICON_FA_SORT_AMOUNT_UP) +
			    " " + getString(descendingOrder ? "sort_descending" : "sort_ascending");
			const float sortButtonWidth = ImGui::CalcTextSize(sortDirectionLabel.c_str()).x +
			                              ImGui::GetStyle().FramePadding.x * 2.0f;
			const float rightWpWidth = (waypointButtonHeight * 2.0f) + sortButtonWidth +
			                           (ImGui::GetStyle().ItemSpacing.x * 2.0f);
			ImGui::SameLine(std::max(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x,
			                         ImGui::GetWindowContentRegionMax().x - rightWpWidth));

			if (UI::transparentButton(ICON_FA_PLUS, ImVec2(waypointButtonHeight, waypointButtonHeight), false))
			{
				Waypoint newWp{ getString("new_waypoint"), context.currentTick };
				context.score.waypoints.push_back(newWp);
				
				std::sort(context.score.waypoints.begin(), context.score.waypoints.end(),
				          [](const Waypoint& a, const Waypoint& b) { return a.tick < b.tick; });

				auto it = std::find_if(context.score.waypoints.begin(), context.score.waypoints.end(),
					[&](const Waypoint& w) { return w.tick == newWp.tick && w.name == newWp.name; });
				
				if (it != context.score.waypoints.end()) {
					renameIndex = std::distance(context.score.waypoints.begin(), it);
					waypointName = newWp.name;
					selectedWaypointIndex = renameIndex;
					focusRenameInput = true;
				}
			}
			ImGui::SameLine();

			bool canDelete = selectedWaypointIndex >= 0 && selectedWaypointIndex < context.score.waypoints.size();
			if (!canDelete) ImGui::BeginDisabled();
			if (UI::transparentButton(ICON_FA_TRASH, ImVec2(waypointButtonHeight, waypointButtonHeight), false))
			{
				if (canDelete)
				{
					context.score.waypoints.erase(context.score.waypoints.begin() + selectedWaypointIndex);
					if (renameIndex == selectedWaypointIndex)
						renameIndex = -1;
					else if (renameIndex > selectedWaypointIndex)
						renameIndex--;
					normalizeWaypointSelection();
				}
			}
			if (!canDelete) ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button(sortDirectionLabel.c_str(), ImVec2(sortButtonWidth, 0.0f)))
				descendingOrder = !descendingOrder;

			ImGui::PopStyleVar();
			ImGui::Separator();

			float windowHeight = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().WindowPadding.y;

			if (ImGui::BeginChild("waypoints_child_window", ImVec2(-1, windowHeight), true))
			{
				ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;
				if (ImGui::BeginTable("waypoints_table", 2, tableFlags))
				{
					ImGui::TableSetupColumn(getString("name"), ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn(getString("time"), ImGuiTableColumnFlags_WidthFixed, 140.0f);
					
					for (int i = 0; i < context.score.waypoints.size(); ++i)
					{
						int index = descendingOrder ? (context.score.waypoints.size() - 1 - i) : i;
						auto& waypoint = context.score.waypoints[index];
						
						ImGui::PushID(index);
						ImGui::TableNextRow(0, waypointButtonHeight);
						
						ImGui::TableSetColumnIndex(0); 

						ImVec2 startPos = ImGui::GetCursorScreenPos();
						bool isSelected = (index == selectedWaypointIndex);

						if (isSelected)
						{
							ImGui::PushStyleColor(ImGuiCol_Header, activeBgColor);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, activeBgColor);
							ImGui::PushStyleColor(ImGuiCol_Text, activeTextColor); 
						}

						if (ImGui::Selectable((std::string("##wp_row_") + std::to_string(index)).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, waypointButtonHeight)))
						{
							selectedWaypointIndex = index;
							context.currentTick = waypoint.tick;
							scrollTimeline(context, waypoint.tick);
						}

						if (isSelected) ImGui::PopStyleColor(3);

						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							renameIndex = index;
							waypointName = waypoint.name;
							focusRenameInput = true;
							selectedWaypointIndex = index;
						}

						ImGui::SetCursorScreenPos(ImVec2(startPos.x + ImGui::GetStyle().FramePadding.x, startPos.y));

						if (renameIndex == index)
						{
							ImGui::SetNextItemWidth(-1);
							if (focusRenameInput)
							{
								ImGui::SetKeyboardFocusHere();
								focusRenameInput = false;
							}
							
							if (ImGui::InputText((std::string("##wprename_") + std::to_string(index)).c_str(), &waypointName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
							{
								context.score.waypoints[index].name = waypointName;
								renameIndex = -1;
							}
							else if (ImGui::IsItemDeactivated())
							{
								if (ImGui::IsKeyPressed(ImGuiKey_Escape)) renameIndex = -1;
								else { context.score.waypoints[index].name = waypointName; renameIndex = -1; }
							}
						}
						else
						{
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(waypoint.name.c_str());
						}

						ImGui::TableSetColumnIndex(1);
						
						int measure = accumulateMeasures(waypoint.tick, TICKS_PER_BEAT, context.score.timeSignatures) + 1;
						float time = accumulateDuration(waypoint.tick, TICKS_PER_BEAT, context.score.tempoChanges);
						char timeStr[64];

						snprintf(timeStr, sizeof(timeStr), "%s %d (%02d:%02d)", getString("measure"), measure, (int)time / 60, (int)std::fmod(time, 60.0f));

						ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetStyle().ItemSpacing.x, startPos.y));
						
						if (isSelected) 
							ImGui::PushStyleColor(ImGuiCol_Text, activeTextColor);
						else 
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f)); 
							
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%s", timeStr);
						ImGui::PopStyleColor();

						ImGui::PopID();
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}
		ImGui::End();
	}}
