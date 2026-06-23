#include "ScoreEditorWindows.h"
#include "Application.h"
#include "AudioTrackUtils.h"
#include "Constants.h"
#include "File.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

#include <algorithm>

namespace MikuMikuWorld
{
	void ScorePropertiesWindow::update(ScoreContext& context)
	{
		if (ImGui::CollapsingHeader(
		        IO::concat(ICON_FA_ALIGN_LEFT, getString("metadata"), " ").c_str(),
		        ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addStringProperty(getString("title"), context.workingData.title);
			UI::addStringProperty(getString("designer"), context.workingData.designer);
			UI::addStringProperty(getString("artist"), context.workingData.artist);

			std::string jacketFile = context.workingData.jacket.getFilename();
			int result = UI::addFileProperty(getString("jacket"), jacketFile);
			if (result == 1)
			{
				context.workingData.jacket.load(jacketFile);
			}
			else if (result == 2)
			{
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Image File";
				fileDialog.filters = { IO::imageFilter, IO::allFilter };
				fileDialog.parentWindowHandle = Application::windowState.windowHandle;

				if (fileDialog.openFile() == IO::FileDialogResult::OK)
					context.workingData.jacket.load(fileDialog.outputFilename);
			}
			context.workingData.jacket.draw();

			std::string filename = context.workingData.musicFilename;
			int filePickResult = UI::addFileProperty(getString("music_file"), filename);
			if (filePickResult == 1 && filename != context.workingData.musicFilename)
			{
				isPendingLoadMusic = true;
				pendingLoadMusicFilename = filename;
			}
			else if (filePickResult == 2)
			{
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Audio File";
				fileDialog.filters = { IO::audioFilter, IO::allFilter };
				fileDialog.parentWindowHandle = Application::windowState.windowHandle;

				if (fileDialog.openFile() == IO::FileDialogResult::OK)
				{
					pendingLoadMusicFilename = fileDialog.outputFilename;
					isPendingLoadMusic = true;
				}
			}
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(
		        IO::concat(ICON_FA_FILE, getString("level_data"), " ").c_str(),
		        ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			float offset = context.workingData.musicOffset;
			UI::addDragFloatProperty(getString("music_offset"), offset, "%.3fms");
			if (offset != context.workingData.musicOffset)
			{
				const float oldOffset = context.workingData.musicOffset;
				context.workingData.musicOffset = offset;
				context.score.metadata.musicOffset = offset;
				if (!context.score.audioTrack.clips.empty())
				{
					if (context.score.audioTrack.clips.size() == 1)
					{
						context.score.audioTrack.clips.front().timelineStartMs = offset;
					}
					else
					{
						const float delta = offset - oldOffset;
						for (AudioClip& clip : context.score.audioTrack.clips)
							clip.timelineStartMs += delta;
					}
				}
				Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
				if (!refreshResult.isOk())
					IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
					               IO::MessageBoxIcon::Warning);
			}

			if (UI::addIntProperty(getString("life_point"), context.workingData.baseLifePoint, 10,
			                       999999))
				context.score.metadata.baseLifePoint = context.workingData.baseLifePoint;
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(
		        IO::concat(ICON_FA_CHART_BAR, getString("statistics"), " ").c_str(),
		        ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addReadOnlyProperty(getString("hispeeds"), context.scoreStats.getHiSpeeds());
			UI::addReadOnlyProperty(getString("taps"), context.scoreStats.getTaps());
			UI::addReadOnlyProperty(getString("flicks"), context.scoreStats.getFlicks());
			UI::addReadOnlyProperty(getString("holds"), context.scoreStats.getHolds());
			UI::addReadOnlyProperty(getString("steps"), context.scoreStats.getSteps());
			UI::addReadOnlyProperty(getString("guides"), context.scoreStats.getGuides());
			UI::addReadOnlyProperty(getString("traces"), context.scoreStats.getTraces());
			UI::addReadOnlyProperty(getString("total"), context.scoreStats.getTotal());
			UI::addReadOnlyProperty(getString("combo"), context.scoreStats.getCombo());
			UI::endPropertyColumns();
		}
	}
}
