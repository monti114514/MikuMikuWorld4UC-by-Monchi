#include "ScoreEditorWindows.h"
#include "NotesPreset.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

namespace MikuMikuWorld
{
	void PresetsWindow::update(ScoreContext& context, PresetManager& presetManager)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_DRAFTING_COMPASS, "presets")))
		{
			int removePattern = -1;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
			                    ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float filterWidth = ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2;

			presetFilter.Draw("##preset_filter",
			                  IO::concat(ICON_FA_SEARCH, getString("search"), " ").c_str(),
			                  filterWidth);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES, UI::btnSmall))
				presetFilter.Clear();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			float presetButtonHeight = ImGui::GetFrameHeight();
			float windowHeight = ImGui::GetContentRegionAvail().y - presetButtonHeight -
			                     ImGui::GetStyle().WindowPadding.y;
			if (ImGui::BeginChild("presets_child_window", ImVec2(-1, windowHeight), true))
			{
				if (!presetManager.presets.size())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					ImGui::TextWrapped("%s", getString("no_presets"));
					ImGui::PopStyleVar();
				}
				else
				{
					for (const auto& [id, preset] : presetManager.presets)
					{
						if (!presetFilter.PassFilter(preset.getName().c_str()))
							continue;

						ImGui::PushID(id);

						if (ImGui::Button(
						        preset.getName().c_str(),
						        ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f,
						               presetButtonHeight)))
							presetManager.applyPreset(id, context);

						if (preset.description.size())
							UI::tooltip(preset.description.c_str());

						ImGui::SameLine();
						if (UI::transparentButton(ICON_FA_TRASH,
						                          ImVec2(UI::btnSmall.x, presetButtonHeight)))
							removePattern = id;

						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			const bool disable =
			    !(context.selectedNotes.size() + context.selectedHiSpeedChanges.size());
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, disable);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * disable));
			if (ImGui::Button(getString("create_preset"), ImVec2(-1, presetButtonHeight)))
				dialogOpen = true;

			ImGui::PopStyleVar();
			ImGui::PopItemFlag();

			if (removePattern != -1)
				presetManager.removePreset(removePattern);
		}

		ImGui::End();

		if (dialogOpen)
		{
			ImGui::OpenPopup(MODAL_TITLE("create_preset"));
			dialogOpen = false;
		}

		if (updateCreationDialog() == DialogResult::Ok)
		{
			presetManager.createPreset(context.score, context.selectedNotes,
			                           context.selectedHiSpeedChanges, presetName, presetDesc);
			presetName.clear();
			presetDesc.clear();
		}
	}

	DialogResult PresetsWindow::updateCreationDialog()
	{
		DialogResult result = DialogResult::None;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("create_preset"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnSmall.y - 2.0f - (padding.y * 2);

			ImGui::Text("%s", getString("preset_name"));
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##preset_name", &presetName);

			ImGui::Text("%s", getString("description"));
			ImGui::InputTextMultiline(
			    "##preset_desc", &presetDesc,
			    { -1, ImGui::GetContentRegionAvail().y - UI::btnSmall.y - 10.0f - padding.y });

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
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !presetName.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !presetName.size()));
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
