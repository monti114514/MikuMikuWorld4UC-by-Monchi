#include "ScoreEditorWindows.h"
#include "../../Application.h"
#include "../../Constants.h"
#include "../../IO.h"
#include "../../UI.h"
#include "../../Utilities.h"

#include <algorithm>

namespace MikuMikuWorld
{
	DialogResult RecentFileNotFoundDialog::update()
	{
		DialogResult result{ DialogResult::None };
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("file_not_found"));
			open = false;
		}

		std::string dialogText = IO::formatString(
		    "%s \"%s\" %s. %s", getString("file_not_found_msg1"), removeFilename.c_str(),
		    getString("file_not_found_msg2"), getString("remove_recent_file_not_found"));

		float maxDialogSizeX{ ImGui::GetMainViewport()->WorkSize.x * 0.80f };
		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
		ImVec2 textSize = ImGui::CalcTextSize(dialogText.c_str());

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(
		    ImVec2(std::min(maxDialogSizeX, textSize.x + (padding.x * 2) + spacing.x), 0),
		    ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("file_not_found"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::TextWrapped("%s", dialogText.c_str());

			// New line to move the buttons a bit down
			ImGui::Text("\n");

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          ImGui::GetFrameHeight() };
			btnSz.x = std::min(btnSz.x, 150.0f);

			// Right align buttons
			ImGui::SetCursorPos(
			    ImVec2(ImGui::GetWindowSize().x - (btnSz.x * 2) - spacing.x - padding.x,
			           ImGui::GetCursorPosY()));

			if (ImGui::Button(getString("yes"), btnSz))
			{
				close();
				result = DialogResult::Yes;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("no"), btnSz))
			{
				close();
				result = DialogResult::No;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	DialogResult UnsavedChangesDialog::update()
	{
		DialogResult result = DialogResult::None;
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("unsaved_changes"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("%s", getString("ask_save"));
			ImGui::Text("%s", getString("warn_unsaved"));

			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

			float btnsHeight = ImGui::GetFrameHeight();
			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - btnsHeight - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              3.0f,
				          btnsHeight };

			if (ImGui::Button(getString("save_changes"), btnSz))
			{
				close();
				result = DialogResult::Yes;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("discard_changes"), btnSz))
			{
				close();
				result = DialogResult::No;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				close();
				result = DialogResult::Cancel;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	DialogResult UpdateAvailableDialog::update()
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("update_available"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("update_available"), NULL,
		                           ImGuiWindowFlags_NoResize))
		{
			ImGui::Text(getString("update_available_description"), latestVersion.c_str());

			float okButtonHeight = ImGui::GetFrameHeight();

			ImGui::SetCursorPos(
			    { ImGui::GetStyle().WindowPadding.x,
			      ImGui::GetWindowSize().y - okButtonHeight - ImGui::GetStyle().WindowPadding.y });

			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          okButtonHeight };

			if (ImGui::Button(getString("yes"), btnSz))
			{
				system("start https://github.com/monti114514/MikuMikuWorld4UC-by-Monchi/releases");
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("no"), btnSz))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::EndPopup();
		}

		return DialogResult::None;
	}

	DialogResult AboutDialog::update()
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("about"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_Appearing);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("about"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text(APP_NAME "\n"
			                     "This application is based on MikuMikuWorld for UntitledCharts.\n\n"
			                     "See LICENSE and NOTICE for license and attribution information.\n\n");
			ImGui::Separator();

			float okButtonHeight = ImGui::GetFrameHeight();

			ImGui::Text("Version %s", Application::getAppVersion().c_str());

			ImGui::SetCursorPos(
			    { ImGui::GetStyle().WindowPadding.x,
			      ImGui::GetWindowSize().y - okButtonHeight - ImGui::GetStyle().WindowPadding.y });

			if (ImGui::Button("OK", { ImGui::GetContentRegionAvail().x, okButtonHeight }))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::EndPopup();
		}

		return DialogResult::None;
	}
}
