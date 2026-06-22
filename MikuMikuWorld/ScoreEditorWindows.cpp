#include "ScoreEditorWindows.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "File.h"
#include "NoteTypes.h"
#include "ScoreContext.h"
#include "AudioTrackUtils.h"
#include "UI.h"
#include "Utilities.h"
#include <algorithm>
#include <climits>
#include <cmath>

namespace MikuMikuWorld
{
	namespace
	{
		constexpr const char* quickSettingLanguage = "language";
		constexpr const char* quickSettingAutoSaveEnabled = "autoSaveEnabled";
		constexpr const char* quickSettingAutoSaveInterval = "autoSaveInterval";
		constexpr const char* quickSettingAutoSaveMaxCount = "autoSaveMaxCount";
		constexpr const char* quickSettingMasterVolume = "masterVolume";
		constexpr const char* quickSettingBgmVolume = "bgmVolume";
		constexpr const char* quickSettingSeVolume = "seVolume";
		constexpr const char* quickSettingBaseTheme = "baseTheme";
		constexpr const char* quickSettingVsync = "vsync";
		constexpr const char* quickSettingMatchNotesSizeToTimeline = "matchNotesSizeToTimeline";
		constexpr const char* quickSettingTimelineWidth = "timelineWidth";
		constexpr const char* quickSettingNotesHeight = "notesHeight";
		constexpr const char* quickSettingDrawWaveform = "drawWaveform";
		constexpr const char* quickSettingReturnToLastTick = "returnToLastTick";
		constexpr const char* quickSettingDrawHiSpeedAutomation = "drawHiSpeedAutomation";
		constexpr const char* quickSettingHiSpeedGraphLimit = "hiSpeedGraphLimit";
		constexpr const char* quickSettingHiSpeedGraphBgOpacity = "hiSpeedGraphBgOpacity";
		constexpr const char* quickSettingFollowCursorInPlayback = "followCursorInPlayback";
		constexpr const char* quickSettingCursorPositionThreshold = "cursorPositionThreshold";
		constexpr const char* quickSettingScrollSpeedNormal = "scrollSpeedNormal";
		constexpr const char* quickSettingScrollSpeedShift = "scrollSpeedShift";
		constexpr const char* quickSettingUseSmoothScrolling = "useSmoothScrolling";
		constexpr const char* quickSettingSmoothScrollingTime = "smoothScrollingTime";
		constexpr const char* quickSettingNotesSe = "notesSe";
		constexpr const char* quickSettingDrawBackground = "drawBackground";
		constexpr const char* quickSettingBackgroundImage = "backgroundImage";
		constexpr const char* quickSettingBackgroundBrightness = "backgroundBrightness";
		constexpr const char* quickSettingLaneOpacity = "laneOpacity";
		constexpr const char* quickSettingShowTickInProperties = "showTickInProperties";
		constexpr const char* quickSettingPreviewDrawToolbar = "previewDrawToolbar";
		constexpr const char* quickSettingPreviewNoteSpeed = "previewNoteSpeed";
		constexpr const char* quickSettingPreviewStageCover = "previewStageCover";
		constexpr const char* quickSettingPreviewHoldAlpha = "previewHoldAlpha";
		constexpr const char* quickSettingPreviewGuideAlpha = "previewGuideAlpha";
		constexpr const char* quickSettingPreviewMirrorScore = "previewMirrorScore";
		constexpr const char* quickSettingPreviewSimultaneousLine = "previewSimultaneousLine";
		constexpr const char* quickSettingPreviewBackgroundBrightness =
		    "previewBackgroundBrightness";
		constexpr const char* quickSettingPreviewStageOpacity = "previewStageOpacity";

		bool settingIs(const char* lhs, const char* rhs) { return std::string(lhs) == rhs; }

		bool isQuickSettingPinned(const char* settingId)
		{
			return std::find(config.pinnedQuickSettings.begin(), config.pinnedQuickSettings.end(),
			                 settingId) != config.pinnedQuickSettings.end();
		}

		void setQuickSettingPinned(const char* settingId, bool pinned)
		{
			auto it = std::find(config.pinnedQuickSettings.begin(),
			                    config.pinnedQuickSettings.end(), settingId);
			if (pinned && it == config.pinnedQuickSettings.end())
				config.pinnedQuickSettings.push_back(settingId);
			else if (!pinned && it != config.pinnedQuickSettings.end())
				config.pinnedQuickSettings.erase(it);
		}

		void applyVolumeSetting(ScoreContext& context, const char* settingId, float value)
		{
			value = std::clamp(value, 0.0f, 1.0f);
			if (settingIs(settingId, quickSettingMasterVolume))
			{
				config.masterVolume = value;
				context.audio.setMasterVolume(value);
			}
			else if (settingIs(settingId, quickSettingBgmVolume))
			{
				config.bgmVolume = value;
				context.audio.setMusicVolume(value);
			}
			else if (settingIs(settingId, quickSettingSeVolume))
			{
				config.seVolume = value;
				context.audio.setSoundEffectsVolume(value);
			}
		}

		float getVolumeSettingValue(const char* settingId)
		{
			if (settingIs(settingId, quickSettingMasterVolume))
				return config.masterVolume;
			if (settingIs(settingId, quickSettingBgmVolume))
				return config.bgmVolume;
			if (settingIs(settingId, quickSettingSeVolume))
				return config.seVolume;
			return 0.0f;
		}

		const char* getQuickSettingLabel(const char* settingId)
		{
			if (settingIs(settingId, quickSettingLanguage))
				return getString("language");
			if (settingIs(settingId, quickSettingAutoSaveEnabled))
				return getString("auto_save_enable");
			if (settingIs(settingId, quickSettingAutoSaveInterval))
				return getString("auto_save_interval");
			if (settingIs(settingId, quickSettingAutoSaveMaxCount))
				return getString("auto_save_count");
			if (settingIs(settingId, quickSettingMasterVolume))
				return getString("volume_master");
			if (settingIs(settingId, quickSettingBgmVolume))
				return getString("volume_bgm");
			if (settingIs(settingId, quickSettingSeVolume))
				return getString("volume_se");
			if (settingIs(settingId, quickSettingBaseTheme))
				return getString("base_theme");
			if (settingIs(settingId, quickSettingVsync))
				return getString("vsync");
			if (settingIs(settingId, quickSettingMatchNotesSizeToTimeline))
				return getString("match_notes_size_to_timeline");
			if (settingIs(settingId, quickSettingTimelineWidth))
				return getString("lane_width");
			if (settingIs(settingId, quickSettingNotesHeight))
				return getString("notes_height");
			if (settingIs(settingId, quickSettingDrawWaveform))
				return getString("draw_waveform");
			if (settingIs(settingId, quickSettingReturnToLastTick))
				return getString("return_to_last_tick");
			if (settingIs(settingId, quickSettingDrawHiSpeedAutomation))
				return getString("draw_hispeed_automation");
			if (settingIs(settingId, quickSettingHiSpeedGraphLimit))
				return getString("hispeed_graph_limit");
			if (settingIs(settingId, quickSettingHiSpeedGraphBgOpacity))
				return getString("hispeed_graph_bg_opacity");
			if (settingIs(settingId, quickSettingFollowCursorInPlayback))
				return getString("cursor_auto_scroll");
			if (settingIs(settingId, quickSettingCursorPositionThreshold))
				return getString("cursor_auto_scroll_amount");
			if (settingIs(settingId, quickSettingScrollSpeedNormal))
				return getString("scroll_speed_normal");
			if (settingIs(settingId, quickSettingScrollSpeedShift))
				return getString("scroll_speed_shift");
			if (settingIs(settingId, quickSettingUseSmoothScrolling))
				return getString("use_smooth_scroll");
			if (settingIs(settingId, quickSettingSmoothScrollingTime))
				return getString("smooth_scroll_time");
			if (settingIs(settingId, quickSettingNotesSe))
				return getString("notes_se");
			if (settingIs(settingId, quickSettingDrawBackground))
				return getString("draw_background");
			if (settingIs(settingId, quickSettingBackgroundImage))
				return getString("background_image");
			if (settingIs(settingId, quickSettingBackgroundBrightness))
				return getString("background_brightnes");
			if (settingIs(settingId, quickSettingLaneOpacity))
				return getString("lanes_opacity");
			if (settingIs(settingId, quickSettingShowTickInProperties))
				return getString("show_tick_in_properties");
			if (settingIs(settingId, quickSettingPreviewDrawToolbar))
				return getString("preview_draw_toolbar");
			if (settingIs(settingId, quickSettingPreviewNoteSpeed))
				return getString("notes_speed");
			if (settingIs(settingId, quickSettingPreviewStageCover))
				return getString("stage_cover");
			if (settingIs(settingId, quickSettingPreviewHoldAlpha))
				return getString("holds_alpha");
			if (settingIs(settingId, quickSettingPreviewGuideAlpha))
				return getString("guides_alpha");
			if (settingIs(settingId, quickSettingPreviewMirrorScore))
				return getString("mirror_score");
			if (settingIs(settingId, quickSettingPreviewSimultaneousLine))
				return getString("simultaneous_lines");
			if (settingIs(settingId, quickSettingPreviewBackgroundBrightness))
				return getString("background_brightnes");
			if (settingIs(settingId, quickSettingPreviewStageOpacity))
				return getString("stage_opacity");
			return "";
		}

		void pinnedPropertyLabel(const char* settingId, const char* label)
		{
			const bool pinned = isQuickSettingPinned(settingId);
			const ImVec4 iconColor = pinned
			                             ? ImGui::GetStyleColorVec4(ImGuiCol_Text)
			                             : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			std::string pinButton = std::string(ICON_FA_THUMBTACK) + "##pin_" + settingId;

			ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
			if (UI::transparentButton(pinButton.c_str(),
			                          ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()),
			                          false))
				setQuickSettingPinned(settingId, !pinned);
			ImGui::PopStyleColor();
			UI::tooltip(getString(pinned ? "unpin_quick_setting" : "pin_quick_setting"));

			ImGui::SameLine();
			ImGui::Text("%s", label);
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-1);
		}

		void addPinnedPercentSliderProperty(ScoreContext& context, const char* settingId,
		                                    const char* label)
		{
			float value = getVolumeSettingValue(settingId);
			const float previousValue = value;

			pinnedPropertyLabel(settingId, label);
			float scaled = value * 100.0f;
			ImGui::SliderFloat(UI::labelID(label), &scaled, 0.0f, 100.0f, "%.0f%%",
			                   ImGuiSliderFlags_AlwaysClamp);
			ImGui::NextColumn();

			value = std::clamp(scaled / 100.0f, 0.0f, 1.0f);
			if (value != previousValue)
				applyVolumeSetting(context, settingId, value);
		}

		bool addPinnedCheckboxProperty(const char* settingId, const char* label, bool& value)
		{
			pinnedPropertyLabel(settingId, label);
			bool edited = ImGui::Checkbox(UI::labelID(label), &value);
			ImGui::NextColumn();
			return edited;
		}

		bool addPinnedIntProperty(const char* settingId, const char* label, int& value,
		                          const char* format = "%d", int lowerBound = 0,
		                          int upperBound = 0)
		{
			pinnedPropertyLabel(settingId, label);
			int step = 1;
			bool edited = ImGui::InputScalar(UI::labelID(label), ImGuiDataType_S32, &value, &step,
			                                 &step, format);
			if (edited && lowerBound != upperBound)
				value = std::clamp(value, lowerBound, upperBound);
			ImGui::NextColumn();
			return edited;
		}

		bool addPinnedFloatProperty(const char* settingId, const char* label, float& value,
		                            const char* format, float lowerBound = 0.0f,
		                            float upperBound = 0.0f)
		{
			pinnedPropertyLabel(settingId, label);
			bool edited = ImGui::InputFloat(UI::labelID(label), &value, 1.0f, 10.0f, format);
			if (edited && lowerBound != upperBound)
				value = std::clamp(value, lowerBound, upperBound);
			ImGui::NextColumn();
			return edited;
		}

		void addPinnedSliderProperty(const char* settingId, const char* label, float& value,
		                             float min, float max, const char* format)
		{
			pinnedPropertyLabel(settingId, label);
			ImGui::SliderFloat(UI::labelID(label), &value, min, max, format,
			                   ImGuiSliderFlags_AlwaysClamp);
			ImGui::NextColumn();
		}

		void addPinnedPercentSliderProperty(const char* settingId, const char* label,
		                                    float& value)
		{
			pinnedPropertyLabel(settingId, label);
			float scaled = value * 100.0f;
			ImGui::SliderFloat(UI::labelID(label), &scaled, 0.0f, 100.0f, "%.0f%%",
			                   ImGuiSliderFlags_AlwaysClamp);
			ImGui::NextColumn();
			value = scaled / 100.0f;
		}

		bool addPinnedSelectProperty(const char* settingId, const char* label, int& value,
		                             const char* const* items, int count)
		{
			pinnedPropertyLabel(settingId, label);
			bool edited = false;
			std::string curr = getString(items[value]);
			if (!curr.size())
				curr = items[value];
			if (ImGui::BeginCombo(UI::labelID(label), curr.c_str()))
			{
				for (int i = 0; i < count; ++i)
				{
					const bool selected = value == i;
					std::string str = getString(items[i]);
					if (!str.size())
						str = items[i];
					if (ImGui::Selectable(str.c_str(), selected))
					{
						value = i;
						edited = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::NextColumn();
			return edited;
		}

		void addPinnedLanguageProperty()
		{
			pinnedPropertyLabel(quickSettingLanguage, getString("language"));
			std::string curr = getString("auto");
			auto langIt = Localization::languages.find(config.language);
			if (langIt != Localization::languages.end())
				curr = langIt->second->getString("language_name");

			if (ImGui::BeginCombo("##language", curr.c_str()))
			{
				if (ImGui::Selectable(getString("auto"), config.language == "auto"))
					config.language = "auto";

				for (const auto& [code, language] : Localization::languages)
				{
					const bool selected = config.language == code;
					std::string str = language->getString("language_name");
					if (ImGui::Selectable(str.c_str(), selected))
						config.language = code;
				}
				ImGui::EndCombo();
			}
			ImGui::NextColumn();
		}

		int addPinnedFileProperty(const char* settingId, const char* label, std::string& value)
		{
			pinnedPropertyLabel(settingId, label);
			int result = 0;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x -
			                        ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::InputTextWithHint(UI::labelID(label), "n/a", &value,
			                             ImGuiInputTextFlags_EnterReturnsTrue))
				result = 1;
			ImGui::SameLine();
			ImGui::PushID(settingId);
			if (ImGui::Button("...", UI::btnSmall))
				result = 2;
			ImGui::PopID();
			ImGui::NextColumn();
			return result;
		}

		bool addQuickSetting(ScoreContext& context, const char* settingId,
		                     bool& backgroundChangePending)
		{
			const char* label = getQuickSettingLabel(settingId);
			if (label[0] == '\0')
				return false;

			if (settingIs(settingId, quickSettingLanguage))
			{
				UI::propertyLabel(label);
				std::string curr = getString("auto");
				auto langIt = Localization::languages.find(config.language);
				if (langIt != Localization::languages.end())
					curr = langIt->second->getString("language_name");

				if (ImGui::BeginCombo("##quick_language", curr.c_str()))
				{
					if (ImGui::Selectable(getString("auto"), config.language == "auto"))
						config.language = "auto";

					for (const auto& [code, language] : Localization::languages)
					{
						const bool selected = config.language == code;
						std::string str = language->getString("language_name");
						if (ImGui::Selectable(str.c_str(), selected))
							config.language = code;
					}
					ImGui::EndCombo();
				}
				ImGui::NextColumn();
			}
			else if (settingIs(settingId, quickSettingAutoSaveEnabled))
				UI::addCheckboxProperty(label, config.autoSaveEnabled);
			else if (settingIs(settingId, quickSettingAutoSaveInterval))
				UI::addIntProperty(label, config.autoSaveInterval);
			else if (settingIs(settingId, quickSettingAutoSaveMaxCount))
				UI::addIntProperty(label, config.autoSaveMaxCount);
			else if (settingIs(settingId, quickSettingMasterVolume) ||
			         settingIs(settingId, quickSettingBgmVolume) ||
			         settingIs(settingId, quickSettingSeVolume))
			{
				float value = getVolumeSettingValue(settingId);
				const float previousValue = value;
				UI::addPercentSliderProperty(label, value);
				if (value != previousValue)
					applyVolumeSetting(context, settingId, value);
			}
			else if (settingIs(settingId, quickSettingBaseTheme))
				UI::addSelectProperty(label, config.baseTheme, baseThemes,
				                      static_cast<int>(BaseTheme::BASE_THEME_MAX));
			else if (settingIs(settingId, quickSettingVsync))
			{
				bool vsync = Application::windowState.vsync;
				UI::addCheckboxProperty(label, Application::windowState.vsync);
				if (vsync != Application::windowState.vsync)
					glfwSwapInterval(Application::windowState.vsync ? 1 : 0);
			}
			else if (settingIs(settingId, quickSettingMatchNotesSizeToTimeline))
				UI::addCheckboxProperty(label, config.matchNotesSizeToTimeline);
			else if (settingIs(settingId, quickSettingTimelineWidth))
				UI::addIntProperty(label, config.timelineWidth, "%dpx", MIN_LANE_WIDTH,
				                   MAX_LANE_WIDTH);
			else if (settingIs(settingId, quickSettingNotesHeight))
			{
				if (config.matchNotesSizeToTimeline)
					UI::beginNextItemDisabled();
				UI::addIntProperty(label, config.notesHeight, "%dpx", MIN_NOTES_HEIGHT,
				                   MAX_NOTES_HEIGHT);
				if (config.matchNotesSizeToTimeline)
					UI::endNextItemDisabled();
			}
			else if (settingIs(settingId, quickSettingDrawWaveform))
				UI::addCheckboxProperty(label, config.drawWaveform);
			else if (settingIs(settingId, quickSettingReturnToLastTick))
				UI::addCheckboxProperty(label, config.returnToLastSelectedTickOnPause);
			else if (settingIs(settingId, quickSettingDrawHiSpeedAutomation))
				UI::addCheckboxProperty(label, config.drawHiSpeedAutomation);
			else if (settingIs(settingId, quickSettingHiSpeedGraphLimit))
				UI::addSliderProperty(label, config.hiSpeedGraphLimit, 1.0f, 10.0f, "%.1fx");
			else if (settingIs(settingId, quickSettingHiSpeedGraphBgOpacity))
				UI::addPercentSliderProperty(label, config.hiSpeedGraphBgOpacity);
			else if (settingIs(settingId, quickSettingFollowCursorInPlayback))
				UI::addCheckboxProperty(label, config.followCursorInPlayback);
			else if (settingIs(settingId, quickSettingCursorPositionThreshold))
				UI::addPercentSliderProperty(label, config.cursorPositionThreshold);
			else if (settingIs(settingId, quickSettingScrollSpeedNormal))
			{
				UI::addFloatProperty(label, config.scrollSpeedNormal, "%.1fx");
				config.scrollSpeedNormal = std::max(0.1f, config.scrollSpeedNormal);
			}
			else if (settingIs(settingId, quickSettingScrollSpeedShift))
			{
				UI::addFloatProperty(label, config.scrollSpeedShift, "%.1fx");
				config.scrollSpeedShift = std::max(0.1f, config.scrollSpeedShift);
			}
			else if (settingIs(settingId, quickSettingUseSmoothScrolling))
				UI::addCheckboxProperty(label, config.useSmoothScrolling);
			else if (settingIs(settingId, quickSettingSmoothScrollingTime))
				UI::addSliderProperty(label, config.smoothScrollingTime, 10.0f, 150.0f,
				                      "%.2fms");
			else if (settingIs(settingId, quickSettingNotesSe))
				UI::addSelectProperty(label, config.seProfileIndex,
				                      Audio::soundEffectsProfileNames,
				                      Audio::soundEffectsProfileCount);
			else if (settingIs(settingId, quickSettingDrawBackground))
				UI::addCheckboxProperty(label, config.drawBackground);
			else if (settingIs(settingId, quickSettingBackgroundImage))
			{
				std::string backgroundFile = config.backgroundImage;
				int result = UI::addFileProperty(label, backgroundFile);
				if (result == 1)
				{
					config.backgroundImage = backgroundFile;
					backgroundChangePending = true;
				}
				else if (result == 2)
				{
					IO::FileDialog fileDialog{};
					fileDialog.title = "Open Image File";
					fileDialog.filters = { IO::imageFilter, IO::allFilter };
					fileDialog.parentWindowHandle = Application::windowState.windowHandle;

					if (fileDialog.openFile() == IO::FileDialogResult::OK)
					{
						config.backgroundImage = fileDialog.outputFilename;
						backgroundChangePending = true;
					}
				}
			}
			else if (settingIs(settingId, quickSettingBackgroundBrightness))
				UI::addPercentSliderProperty(label, config.backgroundBrightness);
			else if (settingIs(settingId, quickSettingLaneOpacity))
				UI::addPercentSliderProperty(label, config.laneOpacity);
			else if (settingIs(settingId, quickSettingShowTickInProperties))
				UI::addCheckboxProperty(label, config.showTickInProperties);
			else if (settingIs(settingId, quickSettingPreviewDrawToolbar))
				UI::addCheckboxProperty(label, config.pvDrawToolbar);
			else if (settingIs(settingId, quickSettingPreviewNoteSpeed))
				UI::addSliderProperty(label, config.pvNoteSpeed, 1, 12, "%.2f");
			else if (settingIs(settingId, quickSettingPreviewStageCover))
				UI::addPercentSliderProperty(label, config.pvStageCover);
			else if (settingIs(settingId, quickSettingPreviewHoldAlpha))
			{
				float holdAlpha = config.pvHoldAlpha * 100.0f;
				UI::addSliderProperty(label, holdAlpha, 10.0f, 100.0f, "%.0f%%");
				config.pvHoldAlpha = holdAlpha / 100.0f;
			}
			else if (settingIs(settingId, quickSettingPreviewGuideAlpha))
			{
				float guideAlpha = config.pvGuideAlpha * 100.0f;
				UI::addSliderProperty(label, guideAlpha, 10.0f, 100.0f, "%.0f%%");
				config.pvGuideAlpha = guideAlpha / 100.0f;
			}
			else if (settingIs(settingId, quickSettingPreviewMirrorScore))
				UI::addCheckboxProperty(label, config.pvMirrorScore);
			else if (settingIs(settingId, quickSettingPreviewSimultaneousLine))
				UI::addCheckboxProperty(label, config.pvSimultaneousLine);
			else if (settingIs(settingId, quickSettingPreviewBackgroundBrightness))
				UI::addPercentSliderProperty(label, config.pvBackgroundBrightness);
			else if (settingIs(settingId, quickSettingPreviewStageOpacity))
				UI::addPercentSliderProperty(label, config.pvStageOpacity);
			else
				return false;

			return true;
		}
	}

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
		        IO::concat(ICON_FA_GAMEPAD, getString("level_data"), " ").c_str(),
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

	void ScoreNotePropertiesWindow::update(ScoreContext& context, int currentDivision)
	{
		auto numSelected = context.selectedNotes.size() + context.selectedHiSpeedChanges.size() +
		                   context.selectedMetaEvents.size();
		if (numSelected == 0)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		Score prev = context.score;
		bool edited = false;

		if (!context.selectedMetaEvents.empty() && context.selectedNotes.empty() &&
		    context.selectedHiSpeedChanges.empty())
		{
			if (context.selectedMetaEvents.size() > 1)
			{
				ImGui::Text("%s", "Multiple meta events selected.");
				return;
			}

			const SelectedMetaEvent selectedMeta = *context.selectedMetaEvents.begin();
			int selectedTick = context.getMetaEventTick(selectedMeta);
			if (selectedTick < 0)
			{
				ImGui::Text("%s", getString("note_properties_not_selected"));
				return;
			}

			if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG, getString("general"), " ").c_str(),
			                            ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				double beat = selectedTick / static_cast<float>(TICKS_PER_BEAT);
				UI::propertyLabel(getString("beat"));
				ImGui::SetNextItemWidth(-1);
				double step = 4.0 / (currentDivision > 0 ? currentDivision : 4);
				double step_fast = step * 4.0;
				bool beatChanged =
				    ImGui::InputDouble(IO::concat("##", getString("beat")).c_str(), &beat, step,
				                       step_fast, "%.3f");
				ImGui::NextColumn();

				if (beatChanged)
				{
					int newTick = std::max(0, static_cast<int>(std::round(beat * TICKS_PER_BEAT)));
					if (selectedMeta.kind == MetaEventKind::Waypoint)
					{
						for (Waypoint& waypoint : context.score.waypoints)
							if (waypoint.runtimeId == selectedMeta.key)
								waypoint.tick = newTick;
						edited = true;
					}
					else if (selectedMeta.kind == MetaEventKind::Bpm)
					{
						bool duplicate = false;
						for (const Tempo& tempo : context.score.tempoChanges)
							if (tempo.runtimeId != selectedMeta.key && tempo.tick == newTick)
								duplicate = true;
						if (!duplicate)
						{
							for (Tempo& tempo : context.score.tempoChanges)
								if (tempo.runtimeId == selectedMeta.key && tempo.tick != 0)
									tempo.tick = newTick;
							std::sort(context.score.tempoChanges.begin(),
							          context.score.tempoChanges.end(),
							          [](const Tempo& a, const Tempo& b)
							          { return a.tick < b.tick; });
							edited = true;
						}
					}
					else if (selectedMeta.kind == MetaEventKind::Skill)
					{
						if (context.score.skills.find(selectedMeta.key) !=
						    context.score.skills.end())
						{
							context.score.skills[selectedMeta.key].tick = newTick;
							edited = true;
						}
					}
					else if (selectedMeta.kind == MetaEventKind::FeverStart)
					{
						context.score.fever.startTick =
						    std::min(newTick, context.score.fever.endTick);
						edited = true;
					}
					else if (selectedMeta.kind == MetaEventKind::FeverEnd)
					{
						context.score.fever.endTick =
						    std::max(newTick, context.score.fever.startTick);
						edited = true;
					}
				}

				if (config.showTickInProperties)
				{
					if (UI::addIntProperty(getString("tick"), selectedTick))
					{
						selectedTick = std::max(0, selectedTick);
						if (selectedMeta.kind == MetaEventKind::Waypoint)
						{
							for (Waypoint& waypoint : context.score.waypoints)
								if (waypoint.runtimeId == selectedMeta.key)
									waypoint.tick = selectedTick;
							edited = true;
						}
						else if (selectedMeta.kind == MetaEventKind::Bpm)
						{
							bool duplicate = false;
							for (const Tempo& tempo : context.score.tempoChanges)
								if (tempo.runtimeId != selectedMeta.key &&
								    tempo.tick == selectedTick)
									duplicate = true;
							if (!duplicate)
							{
								for (Tempo& tempo : context.score.tempoChanges)
									if (tempo.runtimeId == selectedMeta.key && tempo.tick != 0)
										tempo.tick = selectedTick;
								std::sort(context.score.tempoChanges.begin(),
								          context.score.tempoChanges.end(),
								          [](const Tempo& a, const Tempo& b)
								          { return a.tick < b.tick; });
								edited = true;
							}
						}
						else if (selectedMeta.kind == MetaEventKind::Skill)
						{
							if (context.score.skills.find(selectedMeta.key) !=
							    context.score.skills.end())
							{
								context.score.skills[selectedMeta.key].tick = selectedTick;
								edited = true;
							}
						}
					}
				}
				UI::endPropertyColumns();
			}

			if (ImGui::CollapsingHeader("Meta Event", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				switch (selectedMeta.kind)
				{
				case MetaEventKind::Waypoint:
					for (Waypoint& waypoint : context.score.waypoints)
					{
						if (waypoint.runtimeId != selectedMeta.key)
							continue;
						UI::addStringProperty(getString("waypoint_name"), waypoint.name);
						if (ImGui::IsItemDeactivatedAfterEdit())
							edited = true;
					}
					break;
				case MetaEventKind::Bpm:
					for (Tempo& tempo : context.score.tempoChanges)
					{
						if (tempo.runtimeId != selectedMeta.key)
							continue;
						if (UI::addFloatProperty(getString("bpm"), tempo.bpm, "%g"))
						{
							tempo.bpm = std::clamp(tempo.bpm, static_cast<float>(MIN_BPM),
							                       static_cast<float>(MAX_BPM));
							edited = true;
						}
					}
					break;
				case MetaEventKind::TimeSignature:
					if (context.score.timeSignatures.find((int)selectedMeta.key) !=
					    context.score.timeSignatures.end())
					{
						TimeSignature& ts = context.score.timeSignatures[(int)selectedMeta.key];
						int measure = ts.measure;
						if (UI::addIntProperty("Measure", measure, 0, INT_MAX))
						{
							if (measure != ts.measure &&
							    context.score.timeSignatures.find(measure) ==
							        context.score.timeSignatures.end())
							{
								TimeSignature moved = ts;
								context.score.timeSignatures.erase(ts.measure);
								moved.measure = measure;
								context.score.timeSignatures[measure] = moved;
								context.selectedMetaEvents.clear();
								context.selectedMetaEvents.insert(
								    { MetaEventKind::TimeSignature, static_cast<id_t>(measure) });
								edited = true;
							}
						}
						TimeSignature& updatedTs =
						    context.score.timeSignatures[(int)context.selectedMetaEvents.begin()->key];
						if (UI::timeSignatureSelect(updatedTs.numerator, updatedTs.denominator))
						{
							updatedTs.numerator =
							    std::clamp(abs(updatedTs.numerator), MIN_TIME_SIGNATURE,
							               MAX_TIME_SIGNATURE_NUMERATOR);
							updatedTs.denominator =
							    std::clamp(abs(updatedTs.denominator), MIN_TIME_SIGNATURE,
							               MAX_TIME_SIGNATURE_DENOMINATOR);
							edited = true;
						}
					}
					break;
				case MetaEventKind::Skill:
					if (context.score.skills.find(selectedMeta.key) != context.score.skills.end())
					{
						SkillTrigger& skill = context.score.skills[selectedMeta.key];
						int level = skill.level;
						edited |= UI::addSelectProperty(getString("skill_effect"), skill.effect,
						                                skillEffectTypes,
						                                arrayLength(skillEffectTypes));
						if (UI::addIntProperty(getString("skill_level"), level, "Lv.%d", 1, 4))
						{
							skill.level = static_cast<uint8_t>(std::clamp(level, 1, 4));
							edited = true;
						}
					}
					break;
				case MetaEventKind::FeverStart:
				case MetaEventKind::FeverEnd:
					edited |= UI::addIntProperty(getString("fever_start_tick"),
					                             context.score.fever.startTick, -1, INT_MAX);
					edited |= UI::addIntProperty(getString("fever_end_tick"),
					                             context.score.fever.endTick, -1, INT_MAX);
					if (context.score.fever.startTick >= 0 && context.score.fever.endTick >= 0 &&
					    context.score.fever.endTick < context.score.fever.startTick)
					{
						context.score.fever.endTick = context.score.fever.startTick;
					}
					break;
				}
				UI::endPropertyColumns();
			}

			if (edited)
				context.pushHistory("Edited meta event", prev, context.score);
			return;
		}

		int selectedTick;
		int selectedLayer;
		try
		{
			selectedTick =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).tick
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin()).tick;
			selectedLayer =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).layer
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin())
			              .layer;
		}
		catch (const std::out_of_range& e)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG, getString("general"), " ").c_str(),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();

			double beat = selectedTick / static_cast<float>(TICKS_PER_BEAT);
			
			UI::propertyLabel(getString("beat"));
			ImGui::SetNextItemWidth(-1);

			double step = 4.0 / (currentDivision > 0 ? currentDivision : 4);
			double step_fast = step * 4.0;
			
			bool beatChanged = ImGui::InputDouble(IO::concat("##", getString("beat")).c_str(), &beat, step, step_fast, "%.3f");
			ImGui::NextColumn();

			if (beatChanged)
			{
				auto newTick = std::round(beat * TICKS_PER_BEAT); 
				for (auto& id : context.selectedNotes)
				{
					context.score.notes.at(id).tick = newTick;
				}
				for (auto& id : context.selectedHiSpeedChanges)
				{
					context.score.hiSpeedChanges.at(id).tick = newTick;
				}
				edited = true;
			}

			if (config.showTickInProperties)
			{
				if (UI::addIntProperty(getString("tick"), selectedTick))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).tick = selectedTick;
					}
					for (auto& id : context.selectedHiSpeedChanges)
					{
						context.score.hiSpeedChanges.at(id).tick = selectedTick;
					}
					edited = true;
				}
			}

			UI::propertyLabel(getString("layer"));
			const std::string layer_name = context.score.layers[selectedLayer].name;
			if (ImGui::BeginCombo(IO::concat("##", getString("layer")).c_str(), layer_name.c_str()))
			{
				for (int i = 0; i < context.score.layers.size(); i++)
				{
					auto& layer = context.score.layers[i];
					bool selected = selectedLayer == i;
					if (ImGui::Selectable(layer.name.c_str(), selected))
					{
						for (auto& id : context.selectedNotes)
						{
							context.score.notes.at(id).layer = i;
						}
						for (auto& id : context.selectedHiSpeedChanges)
						{
							context.score.hiSpeedChanges.at(id).layer = i;
						}
						edited = true;
					}
				}
				ImGui::EndCombo();
			}

			UI::endPropertyColumns();
		}

		if (context.selectedNotes.size() >= 1)
		{
			bool isGuide = false;

			bool multipleHold = false;
			int holdIndex = -1;
			for (id_t id : context.selectedNotes)
			{
				const Note& n = context.score.notes.at(id);
				if (n.isHold())
				{
					auto prevHoldIndex = holdIndex;
					if (n.getType() == NoteType::Hold)
					{
						holdIndex = id;
					}
					else
					{
						holdIndex = n.parentID;
					}
					if (prevHoldIndex != -1 && prevHoldIndex != holdIndex)
					{
						multipleHold = true;
						break;
					}
				}
			}

			Note& note = context.score.notes.at(*context.selectedNotes.begin());
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_COG, getString("note_properties_note"), " ").c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				if (UI::addFloatProperty(getString("lane"), note.lane, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).lane = note.lane;
					}
					edited = true;
				}

				if (UI::addFloatProperty(getString("width"), note.width, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& localNote = context.score.notes.at(id);
						if (localNote.isHold())
						{
							auto& hold = context.score.holdNotes.at(
							    localNote.parentID == -1 ? id : localNote.parentID);
							if (!((localNote.getType() == NoteType::Hold &&
							       hold.startType == HoldNoteType::Normal) ||
							      (localNote.getType() == NoteType::HoldMid &&
							       hold.steps.at(findHoldStep(hold, localNote.ID)).type ==
							           HoldStepType::Normal) ||
							      (localNote.getType() == NoteType::HoldEnd &&
							       hold.endType == HoldNoteType::Normal)))
							{
								localNote.width = std::max(0.0f, note.width);
								continue;
							}
						}

						localNote.width = std::max(0.5f, note.width);
					}
					edited = true;
				}
			}

			if (context.hasHoldInSelection())
			{
				if (!multipleHold)
				{
					auto& hold = context.score.holdNotes.at(holdIndex);
					auto& holdStart = context.score.notes.at(hold.start.ID);

					isGuide = hold.isGuide();
					if (note.width == 0 && !isGuide)
					{
						if (note.getType() == NoteType::Hold)
						{
							hold.startType = HoldNoteType::Hidden;
						}
						else if (note.getType() == NoteType::HoldEnd)
						{
							hold.endType = HoldNoteType::Hidden;
						}
					}

					if (!isGuide)
					{
						if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
						{
							if (UI::addCheckboxProperty(getString("trace"), note.friction))
							{
								for (auto& id : context.selectedNotes)
								{
									auto& n = context.score.notes.at(id);
									if (n.canTrace())
									{
										n.friction = note.friction;
									}
								}
								edited = true;
							}
						}
						if (UI::addCheckboxProperty(getString("critical"), note.critical))
						{
							context.score.notes.at(hold.start.ID).critical = note.critical;
							for (auto& step : hold.steps)
							{
								context.score.notes.at(step.ID).critical = note.critical;
							}
							context.score.notes.at(hold.end).critical = note.critical;
							for (auto& id : context.selectedNotes)
							{
								context.score.notes.at(id).critical = note.critical;
							}
							edited = true;
						}

						bool selectingAnyDummy = !std::all_of(
						    context.selectedNotes.begin(), context.selectedNotes.end(),
						    [&](id_t id)
						    {
							    auto& n = context.score.notes.at(id);
							    if (!n.isHold())
								    return false;
							    auto& h = context.score.holdNotes.at(
							        n.getType() == NoteType::Hold ? n.ID : n.parentID);
							    switch (n.getType())
							    {
							    case NoteType::Hold:
								    return h.startType == HoldNoteType::Hidden;
							    case NoteType::HoldEnd:
								    return h.endType == HoldNoteType::Hidden;
							    case NoteType::HoldMid:
								    return h[findHoldStep(h, id)].type == HoldStepType::Hidden;
							    }
							    return false;
						    });

						if (selectingAnyDummy &&
						    UI::addCheckboxProperty(getString("dummy"), note.dummy))
						{
							for (auto& id : context.selectedNotes)
							{
								auto& n = context.score.notes.at(id);
								n.dummy = note.dummy;
								if (n.dummy)
									n.soundEffect = SoundEffectType::Default;
							}
							edited = true;
						}
					}

					if (note.getType() == NoteType::HoldEnd && !isGuide)
					{
						if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick,
						                                       flickTypes, arrayLength(flickTypes)))
						{
							context.score.notes.at(hold.start.ID).flick = note.flick;
							for (auto& id : context.selectedNotes)
							{
								auto& n = context.score.notes.at(id);
								if (!n.isHold())
								{
									n.flick = note.flick;
								}
							}
						}
					}
				}
			}
			else
			{
				if (UI::addCheckboxProperty(getString("trace"), note.friction))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canTrace())
						{
							n.friction = note.friction;
						}
					}
					edited = true;
				}
				if (UI::addCheckboxProperty(getString("critical"), note.critical))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).critical = note.critical;
					}
					edited = true;
				}
				if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick, flickTypes,
				                                       arrayLength(flickTypes)))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canFlick())
						{
							n.flick = note.flick;
						}
					}
					edited = true;
				}

				if (UI::addCheckboxProperty(getString("dummy"), note.dummy))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						n.dummy = note.dummy;
						if (n.dummy)
							n.soundEffect = SoundEffectType::Default;
					}
					edited = true;
				}
			}

			bool hasEditableSoundEffect = false;
			SoundEffectType soundEffect = note.soundEffect;
			for (auto id : context.selectedNotes)
			{
				auto& n = context.score.notes.at(id);
				if (!n.canSoundEffect())
					continue;
				if (!hasEditableSoundEffect)
					soundEffect = n.soundEffect;
				hasEditableSoundEffect = true;
			}

			if (hasEditableSoundEffect &&
			    UI::addSelectProperty(getString("sound_effect"), soundEffect,
			                          soundEffectTypes, arrayLength(soundEffectTypes)))
			{
				for (auto& id : context.selectedNotes)
				{
					auto& n = context.score.notes.at(id);
					if (n.canSoundEffect())
						n.soundEffect = soundEffect;
				}
				edited = true;
			}

			UI::endPropertyColumns();
			if (context.hasHoldInSelection())
			{
				if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG,
				                                       isGuide
				                                           ? getString("note_properties_guide")
				                                           : getString("note_properties_hold_note"),
				                                       " ")
				                                .c_str(),
				                            ImGuiTreeNodeFlags_DefaultOpen))
				{

					if (multipleHold)
					{
						ImGui::Text("%s", getString("note_properties_many_hold_notes"));
					}
					else
					{
						auto& hold = context.score.holdNotes.at(holdIndex);
						auto& holdStart = context.score.notes.at(hold.start.ID);

						int stepIndex = findHoldStep(hold, note.ID);

						UI::beginPropertyColumns();

						bool hasEaseable = false;
						Note easeableNote;
						bool hasStepType = false;
						Note stepTypeNote;
						bool hasStepLayer = false;
						Note stepLayerNote;
						for (auto id : context.selectedNotes)
						{
							auto& n = context.score.notes.at(id);
							if (n.hasEase())
							{
								if (!hasEaseable)
								{
									easeableNote = n;
								}
								hasEaseable = true;
							}
							if (context.score.notes.at(id).getType() == NoteType::HoldMid)
							{
								if (!hasStepType)
								{
									stepTypeNote = n;
								}
								hasStepType = true;
							}
							if (n.getType() == NoteType::Hold || n.getType() == NoteType::HoldMid)
							{
								if (!hasStepLayer)
								{
									stepLayerNote = n;
								}
								hasStepLayer = true;
							}
						}

						if (hasEaseable)
						{
							int stepIndex = findHoldStep(hold, easeableNote.ID);
							auto ease = easeableNote.getType() == NoteType::Hold
							                ? hold.start.ease
							                : hold.steps.at(stepIndex == -1 ? 0 : stepIndex).ease;
							if (UI::addSelectProperty(getString("ease_type"), ease, easeTypes,
							                          arrayLength(easeTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.hasEase())
									{
										if (note.getType() == NoteType::Hold)
										{
											hold.start.ease = ease;
										}
										else
										{
											auto& step = hold.steps.at(findHoldStep(hold, note.ID));
											step.ease = ease;
										}
									}
								}
							}
						}

						if (hasStepType)
						{
							int stepIndex = findHoldStep(hold, stepTypeNote.ID);
							auto stepType = hold.steps.at(stepIndex).type;

							if (UI::addSelectProperty(getString("step_type"), stepType, stepTypes,
							                          arrayLength(stepTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::HoldMid)
									{
										auto& step = hold.steps.at(findHoldStep(hold, note.ID));
										step.type = stepType;
									}
								}
							}
						}

						if (hasStepLayer)
						{
							HoldStepLayer stepLayer = HoldStepLayer::Top;
							if (stepLayerNote.getType() == NoteType::Hold)
							{
								stepLayer = hold.start.layer;
							}
							else
							{
								int stepIndex = findHoldStep(hold, stepLayerNote.ID);
								if (stepIndex != -1)
								{
									stepLayer = hold.steps.at(stepIndex).layer;
								}
							}

							if (UI::addSelectProperty(getString("hold_step_layer"), stepLayer,
							                          holdStepLayers, arrayLength(holdStepLayers)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::Hold)
									{
										hold.start.layer = stepLayer;
									}
									else if (note.getType() == NoteType::HoldMid)
									{
										int localStepIndex = findHoldStep(hold, note.ID);
										if (localStepIndex != -1)
										{
											hold.steps.at(localStepIndex).layer = stepLayer;
										}
									}
								}
								edited = true;
							}
						}

						if (isGuide)
						{
							edited |= UI::addSelectProperty(getString("guide_color"),
							                                hold.guideColor, guideColorsForString,
							                                arrayLength(guideColors));
							edited |= UI::addSelectProperty(getString("fade_type"), hold.fadeType,
							                                fadeTypes, arrayLength(fadeTypes));
						}
						else
						{
							auto holdType =
							    note.getType() == NoteType::Hold ? hold.startType : hold.endType;
							if (UI::addSelectProperty(getString("hold_type"), holdType, holdTypes,
							                          2))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::Hold)
									{
										hold.startType = holdType;
									}
									else
									{
										hold.endType = holdType;
									}
								}
								edited = true;
							}

							if (UI::addCheckboxProperty(getString("hold_dummy"), hold.dummy))
							{
								edited = true;
							}
						}
					}
				}

				UI::endPropertyColumns();
			}
		}
		if (context.selectedHiSpeedChanges.size() >= 1)
		{
			bool speedEdited = false;
			HiSpeedChange& editHiSpeed =
			    context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin());
			float speed = editHiSpeed.speed;
			float skip = editHiSpeed.skips;
			auto ease = editHiSpeed.ease;
			bool hideNotes = editHiSpeed.hideNotes;
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_FAST_FORWARD, getString("note_properties_hi_speed"), " ")
			            .c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				speedEdited |= UI::addFloatProperty(getString("hi_speed_speed"), speed, "%.3f");
				speedEdited |=
				    UI::addSelectProperty(getString("hi_speed_ease"), ease, hiSpeedEaseNames,
				                          arrayLength(hiSpeedEaseNames));

				speedEdited |=
				    UI::addFloatProperty(getString("hi_speed_skip_beat"), skip, 
					IO::formatString("%%.3f %s", getString("beat")).c_str());

				speedEdited |= UI::addCheckboxProperty(getString("hi_speed_hide_notes"), hideNotes);
				UI::endPropertyColumns();
			}

			if (speedEdited)
			{
				edited = true;
				for (auto& id : context.selectedHiSpeedChanges)
				{
					HiSpeedChange& hiSpeed = context.score.hiSpeedChanges.at(id);
					hiSpeed.speed = speed;
					hiSpeed.skips = skip;
					hiSpeed.ease = ease;
					hiSpeed.hideNotes = hideNotes;
				}
			}
		}

		if (edited)
			context.pushHistory("Edited object", prev, context.score);
	}

	void ScoreOptionsWindow::update(ScoreContext& context, EditArgs& edit, TimelineMode currentMode,
	                                bool& backgroundChangePending)
	{
		if (ImGui::CollapsingHeader(getString("edit_assist_settings"),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			if (UI::addIntProperty(getString("lane_extension"), context.workingData.laneExtension,
			                       0, 100))
				context.score.metadata.laneExtension = context.workingData.laneExtension;

			bool audioLocked = context.score.audioTrack.locked;
			if (UI::addCheckboxProperty(getString("audio_lock"), audioLocked))
			{
				Score prev = context.score;
				context.score.audioTrack.locked = audioLocked;
				context.pushHistory("Toggle Audio Lock", prev, context.score);
			}

			switch (currentMode)
			{
			default:
			case TimelineMode::InsertTap:
			case TimelineMode::InsertDamage:
			case TimelineMode::MakeCritical:
			case TimelineMode::MakeFriction:
			case TimelineMode::MakeDummy:
				UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
				                   MAX_NOTE_WIDTH);
				break;
			case TimelineMode::InsertFlick:
				UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
				                   MAX_NOTE_WIDTH);
				UI::addSelectProperty<FlickType>(getString("flick"), edit.flickType, flickTypes,
				                                 arrayLength(flickTypes));
				break;
			case TimelineMode::InsertLong:
				UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
				                   MAX_NOTE_WIDTH);
				UI::addSelectProperty<HoldEndType>(getString("hold_start_type"),
				                                   edit.holdStartType, holdEndTypes,
				                                   arrayLength(holdEndTypes));
				UI::addSelectProperty<HoldEndType>(getString("hold_end_type"),
				                                   edit.holdEndType, holdEndTypes,
				                                   arrayLength(holdEndTypes));
				UI::addSelectProperty(getString("ease_type"), edit.easeType, easeTypes,
				                      arrayLength(easeTypes));
				break;
			case TimelineMode::InsertLongMid:
				UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
				                   MAX_NOTE_WIDTH);
				UI::addSelectProperty(getString("step_type"), edit.stepType, stepTypes,
				                      arrayLength(stepTypes));
				break;
			case TimelineMode::InsertGuide:
				UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
				                   MAX_NOTE_WIDTH);
				UI::addSelectProperty(getString("ease_type"), edit.easeType, easeTypes,
				                      arrayLength(easeTypes));
				UI::addSelectProperty<GuideColor>(getString("guide_color"), edit.colorType,
				                                  guideColorsForString, arrayLength(guideColors));
				UI::addSelectProperty<FadeType>(getString("fade_type"), edit.fadeType, fadeTypes,
				                                arrayLength(fadeTypes));
				break;
			case TimelineMode::InsertBPM:
				UI::addFloatProperty(getString("bpm"), edit.bpm, "%g BPM");
				edit.bpm = std::clamp(edit.bpm, MIN_BPM, MAX_BPM);
				break;

			case TimelineMode::InsertTimeSign:
				UI::timeSignatureSelect(edit.timeSignatureNumerator,
				                        edit.timeSignatureDenominator);
				break;

			case TimelineMode::InsertHiSpeed:
				UI::addFloatProperty(getString("hi_speed_speed"), edit.hiSpeed, "%gx");
				UI::addSelectProperty(getString("hi_speed_ease"), edit.hiSpeedEase,
				                      hiSpeedEaseNames, arrayLength(hiSpeedEaseNames));
				UI::addFloatProperty(
				    getString("hi_speed_skip_beat"), edit.hiSpeedSkip,
				    IO::formatString("%%.3f %s", getString("beat")).c_str());
				UI::addCheckboxProperty(getString("hi_speed_hide_notes"), edit.hiSpeedHideNotes);
				break;
			}
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(getString("pinned_settings"),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			int shownSettings = 0;
			for (const std::string& settingId : config.pinnedQuickSettings)
				shownSettings +=
				    addQuickSetting(context, settingId.c_str(), backgroundChangePending) ? 1 : 0;
			UI::endPropertyColumns();

			if (shownSettings == 0)
				ImGui::TextDisabled("%s", getString("no_pinned_settings"));
		}
	}

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

	void DebugWindow::update(ScoreContext& context, ScoreEditorTimeline& timeline)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_BUG, "debug")))
		{
			constexpr ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
			constexpr ImGuiTreeNodeFlags treeNodeFlags = headerFlags | ImGuiTreeNodeFlags_Framed;
			if (ImGui::TreeNodeEx("Audio", treeNodeFlags))
			{
				if (ImGui::CollapsingHeader("Engine", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Sample Rate", context.audio.getDeviceSampleRate());
					UI::addReadOnlyProperty("Channel Count", context.audio.getDeviceChannelCount());
					UI::addReadOnlyProperty(
					    "Latency",
					    IO::formatString("%.2fms", context.audio.getDeviceLatency() * 1000));
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Music", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Music Initialized",
					                        boolToString(context.audio.isMusicInitialized()));
					UI::addReadOnlyProperty("Music Filename", context.audio.musicBuffer.name);

					float musicTime = context.audio.getMusicPosition(),
					      musicLength = context.audio.getMusicLength();
					int musicTimeSeconds = static_cast<int>(musicTime),
					    musicLengthSeconds = static_cast<int>(musicLength);
					UI::addReadOnlyProperty(
					    "Music Time",
					    IO::formatString(
					        "%02d:%02d/%02d:%02d", musicTimeSeconds,
					        static_cast<int>((musicTime - musicTimeSeconds) * 100),
					        musicLengthSeconds,
					        static_cast<int>((musicLength - musicLengthSeconds) * 100)));

					UI::addReadOnlyProperty("Sample Rate", context.audio.musicBuffer.sampleRate);
					UI::addReadOnlyProperty("Effective Sample Rate",
					                        context.audio.musicBuffer.effectiveSampleRate);
					UI::addReadOnlyProperty("Channel Count",
					                        context.audio.musicBuffer.channelCount);
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Waveform", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Waveform L",
					                        boolToString(!context.waveformL.isEmpty()));
					UI::addReadOnlyProperty("Waveform L Mip Count",
					                        context.waveformL.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform L Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::addReadOnlyProperty("Waveform R",
					                        boolToString(!context.waveformR.isEmpty()));
					UI::addReadOnlyProperty("Waveform R Mip Count",
					                        context.waveformR.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform R Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::endPropertyColumns();

					if (ImGui::Button("Re-Generate Waveform", { -1, UI::btnSmall.y }))
					{
						context.waveformL.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 0);
						context.waveformR.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 1);
					}
				}

				if (ImGui::CollapsingHeader("Sound Test", headerFlags))
				{
					constexpr ImGuiTableFlags tableFlags =
					    ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
					    ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
					    ImGuiTableFlags_RowBg;

					const int rowHeight = ImGui::GetFrameHeight() + 5;

					if (ImGui::BeginTable("##sound_test_table", 3, tableFlags, { -1, 200 }))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("Duration (sec)", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Play/Stop", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableHeadersRow();

						for (size_t i = 0;
						     i < arrayLength(SE_NAMES) * Audio::soundEffectsProfileCount; i++)
						{
							Audio::SoundInstance& sound = context.audio.debugSounds[i];

							ImGui::PushID(i);
							ImGui::TableNextRow(0, rowHeight);
							ImGui::TableSetColumnIndex(0);

							float ratio = sound.getCurrentFrame() /
							              static_cast<float>(sound.getLengthInFrames());
							if (!sound.isPlaying())
								ratio = 0.0f;

							ImGui::ProgressBar(ratio, { -1, 0 }, sound.name.c_str());

							float duration = sound.getDuration();
							int durationSecondsOnly = duration;
							float time = sound.isPlaying() ? sound.getCurrentTime() : 0.0f;
							int timeSecondsOnly = time;
							ImGui::TableSetColumnIndex(1);
							ImGui::Text("%02d:%02d/%02d:%02d", timeSecondsOnly,
							            static_cast<int>((time - timeSecondsOnly) * 100),
							            durationSecondsOnly,
							            static_cast<int>((duration - durationSecondsOnly) * 100));

							ImGui::TableSetColumnIndex(2);
							if (UI::transparentButton(ICON_FA_PLAY, UI::btnSmall))
							{
								sound.seek(0);
								sound.play();
							}

							ImGui::SameLine();
							ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
							ImGui::SameLine();
							if (UI::transparentButton(ICON_FA_STOP, UI::btnSmall))
								sound.stop();

							ImGui::PopID();
						}

						ImGui::EndTable();
					}
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Timeline", treeNodeFlags))
			{
				timeline.debug(context);
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	void SettingsWindow::updateKeyConfig(MultiInputBinding* bindings[], int count)
	{
		ImVec2 size = ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.7);
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter |
		                                   ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY |
		                                   ImGuiTableFlags_RowBg;

		const ImGuiSelectableFlags selectionFlags =
		    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
		int rowHeight = ImGui::GetFrameHeight() + 5;

		if (ImGui::BeginTable("##commands_table", 2, tableFlags, size))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn(getString("action"));
			ImGui::TableSetupColumn(getString("keys"));
			ImGui::TableHeadersRow();

			for (int i = 0; i < count; ++i)
			{
				ImGui::TableNextRow(0, rowHeight);

				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(i);
				if (ImGui::Selectable(getString(bindings[i]->name), i == selectedBindingIndex,
				                      selectionFlags))
					selectedBindingIndex = i;

				ImGui::PopID();
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", ToFullShortcutsString(*bindings[i]).c_str());
			}
			ImGui::EndTable();
		}
		ImGui::Separator();

		if (selectedBindingIndex > -1 && selectedBindingIndex < count)
		{
			int deleteBinding = -1;
			int moveIndex = -1;
			int moveDirection = 0;
			const bool canAdd = bindings[selectedBindingIndex]->count < 4;

			const float btnHeight = ImGui::GetFrameHeight();

			UI::beginPropertyColumns();
			ImGui::Text("%s", getString(bindings[selectedBindingIndex]->name));
			ImGui::NextColumn();

			if (!canAdd)
				UI::beginNextItemDisabled();

			if (ImGui::Button(getString("add"), { -1, btnHeight }))
				bindings[selectedBindingIndex]->addBinding(InputBinding{});

			if (!canAdd)
				UI::endNextItemDisabled();

			UI::endPropertyColumns();

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
			ImGui::BeginChild("##binding_keys_edit_window", ImVec2(-1, -1), true);

			float btnWidth = (UI::btnSmall.x * 2) + 100 + (ImGui::GetStyle().ItemSpacing.x * 3);
			for (int b = 0; b < bindings[selectedBindingIndex]->count; ++b)
			{
				const bool canMoveDown = !(bindings[selectedBindingIndex]->count <= b + 1);
				const bool canMoveUp = !(b < 1);
				ImGui::PushID(b);

				std::string buttonText =
				    ToShortcutString(bindings[selectedBindingIndex]->bindings[b]);
				;
				if (!buttonText.size())
					buttonText = getString("none");

				if (listeningForInput && editBindingIndex == b)
					buttonText = getString("cmd_key_listen");

				if (ImGui::Button(buttonText.c_str(),
				                  ImVec2(ImGui::GetContentRegionAvail().x - btnWidth, btnHeight)))
				{
					listeningForInput = true;
					inputTimer.reset();
					editBindingIndex = b;
				}

				ImGui::SameLine();
				if (!canMoveUp)
					UI::beginNextItemDisabled();
				if (ImGui::Button(ICON_FA_CARET_UP, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = -1;
				}
				if (!canMoveUp)
					UI::endNextItemDisabled();

				ImGui::SameLine();
				if (!canMoveDown)
					UI::beginNextItemDisabled();
				if (ImGui::Button(ICON_FA_CARET_DOWN, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = 1;
				}
				if (!canMoveDown)
					UI::endNextItemDisabled();

				ImGui::SameLine();
				if (ImGui::Button(getString("remove"), { -1, btnHeight }))
					deleteBinding = b;

				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			if (moveIndex > -1)
			{
				listeningForInput = false;
				if (moveDirection == -1)
					bindings[selectedBindingIndex]->moveUp(moveIndex);
				if (moveDirection == 1)
					bindings[selectedBindingIndex]->moveDown(moveIndex);
			}

			if (deleteBinding > -1)
			{
				listeningForInput = false;
				bindings[selectedBindingIndex]->removeAt(deleteBinding);
			}
		}

		if (listeningForInput)
		{
			if (inputTimer.elapsed() >= inputTimeoutSeconds)
			{
				listeningForInput = false;
				editBindingIndex = -1;
			}
			else
			{
				for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_MouseLeft; ++key)
				{
					bool isCtrl = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
					              key == ImGuiKey_ModCtrl;
					bool isShift = key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
					               key == ImGuiKey_ModShift;
					bool isAlt = key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
					             key == ImGuiKey_ModAlt;
					bool isSuper = key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper ||
					               key == ImGuiKey_ModSuper;

					// execute if a non-modifier key is tapped
					if (ImGui::IsKeyPressed((ImGuiKey)key) && !isCtrl && !isShift && !isAlt &&
					    !isSuper)
					{
						bindings[selectedBindingIndex]->bindings[editBindingIndex] =
						    InputBinding((ImGuiKey)key, (ImGuiModFlags_)ImGui::GetIO().KeyMods);
						listeningForInput = false;
						editBindingIndex = -1;
					}
				}
			}
		}
	}

	DialogResult SettingsWindow::update(ScoreContext& context)
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("settings"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(750, 600), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));

		if (ImGui::BeginPopupModal(MODAL_TITLE("settings"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			ImVec2 confirmBtnPos =
			    ImGui::GetWindowSize() + ImVec2(-100, -UI::btnNormal.y) - padding;
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() -
			                                          ImVec2(0, UI::btnNormal.y + padding.y));

			if (ImGui::BeginTabBar("##settings_tabs"))
			{
				if (ImGui::BeginTabItem(IMGUI_TITLE("", "general")))
				{
					if (ImGui::CollapsingHeader(getString("language"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedLanguageProperty();
						UI::endPropertyColumns();
					}

					// if (ImGui::CollapsingHeader(getString("file"),
					// ImGuiTreeNodeFlags_DefaultOpen))
					//{
					//	UI::beginPropertyColumns();
					//	UI::addCheckboxProperty(getString("minify"), config.minifyOutput);
					//	UI::endPropertyColumns();
					// }

					if (ImGui::CollapsingHeader(getString("auto_save"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingAutoSaveEnabled,
						                         getString("auto_save_enable"),
						                         config.autoSaveEnabled);
						addPinnedIntProperty(quickSettingAutoSaveInterval,
						                     getString("auto_save_interval"),
						                     config.autoSaveInterval);
						addPinnedIntProperty(quickSettingAutoSaveMaxCount,
						                     getString("auto_save_count"), config.autoSaveMaxCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("audio"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedPercentSliderProperty(context, quickSettingMasterVolume,
						                               getString("volume_master"));
						addPinnedPercentSliderProperty(context, quickSettingBgmVolume,
						                               getString("volume_bgm"));
						addPinnedPercentSliderProperty(context, quickSettingSeVolume,
						                               getString("volume_se"));
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("theme"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						int baseTheme = static_cast<int>(config.baseTheme);
						if (addPinnedSelectProperty(quickSettingBaseTheme, getString("base_theme"),
						                            baseTheme, baseThemes,
						                            static_cast<int>(BaseTheme::BASE_THEME_MAX)))
							config.baseTheme = static_cast<BaseTheme>(baseTheme);
						UI::endPropertyColumns();

						ImGui::TextWrapped("%s", getString("accent_color_help"));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						                    ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

						for (int i = 0; i < UI::accentColors.size(); ++i)
						{
							bool apply = false;
							std::string id = i == config.accentColor ? ICON_FA_CHECK
							                 : i == 0                ? "C"
							                                         : "##" + std::to_string(i);
							ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i]);

							apply = ImGui::Button(id.c_str(), UI::btnNormal);

							ImGui::PopStyleColor(3);

							if ((i < UI::accentColors.size() - 1) &&
							    ImGui::GetCursorPosX() <
							        ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
								ImGui::SameLine();

							if (apply)
								config.accentColor = i;
						}
						ImGui::PopStyleVar(2);

						ImVec4& customColor = UI::accentColors[0];
						float col[]{ customColor.x, customColor.y, customColor.z };
						static ColorDisplay displayMode = ColorDisplay::HEX;

						ImGui::Separator();
						ImGui::Text("%s", getString("select_accent_color"));
						UI::beginPropertyColumns();
						UI::propertyLabel(getString("display_mode"));
						if (ImGui::BeginCombo("##color_display_mode",
						                      colorDisplayStr[(int)displayMode]))
						{
							for (int i = 0; i < 3; ++i)
							{
								const bool selected = (int)displayMode == i;
								if (ImGui::Selectable(colorDisplayStr[i], selected))
									displayMode = (ColorDisplay)i;
							}

							ImGui::EndCombo();
						}
						ImGui::NextColumn();
						UI::propertyLabel(getString("custom_color"));

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						                    ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGuiColorEditFlags flags = 1 << (20 + (int)displayMode);
						if (ImGui::ColorEdit3("##custom_accent_color", col, flags))
							customColor.x = col[0];
						customColor.y = col[1];
						customColor.z = col[2];

						UI::endPropertyColumns();
						ImGui::PopStyleVar();
					}

					if (ImGui::CollapsingHeader(getString("video"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						bool vsync = Application::windowState.vsync;
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingVsync, getString("vsync"),
						                         Application::windowState.vsync);
						UI::endPropertyColumns();

						if (vsync != Application::windowState.vsync)
							glfwSwapInterval(Application::windowState.vsync ? 1 : 0);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "timeline")))
				{
					if (ImGui::CollapsingHeader(getString("timeline"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingMatchNotesSizeToTimeline,
						                         getString("match_notes_size_to_timeline"),
						                         config.matchNotesSizeToTimeline);
						addPinnedIntProperty(quickSettingTimelineWidth, getString("lane_width"),
						                     config.timelineWidth, "%dpx", MIN_LANE_WIDTH,
						                     MAX_LANE_WIDTH);

						if (config.matchNotesSizeToTimeline)
							UI::beginNextItemDisabled();
						addPinnedIntProperty(quickSettingNotesHeight, getString("notes_height"),
						                     config.notesHeight, "%dpx", MIN_NOTES_HEIGHT,
						                     MAX_NOTES_HEIGHT);
						if (config.matchNotesSizeToTimeline)
							UI::endNextItemDisabled();
						ImGui::Separator();

						addPinnedCheckboxProperty(quickSettingDrawWaveform,
						                         getString("draw_waveform"), config.drawWaveform);
						addPinnedCheckboxProperty(quickSettingReturnToLastTick,
						                         getString("return_to_last_tick"),
						                         config.returnToLastSelectedTickOnPause);
						ImGui::Separator();

						addPinnedCheckboxProperty(quickSettingDrawHiSpeedAutomation,
						                         getString("draw_hispeed_automation"),
						                         config.drawHiSpeedAutomation);
						addPinnedSliderProperty(quickSettingHiSpeedGraphLimit,
						                        getString("hispeed_graph_limit"),
						                        config.hiSpeedGraphLimit, 1.0f, 10.0f, "%.1fx");
						addPinnedPercentSliderProperty(quickSettingHiSpeedGraphBgOpacity,
						                              getString("hispeed_graph_bg_opacity"),
						                              config.hiSpeedGraphBgOpacity);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("scrolling"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingFollowCursorInPlayback,
						                         getString("cursor_auto_scroll"),
						                         config.followCursorInPlayback);
						addPinnedPercentSliderProperty(quickSettingCursorPositionThreshold,
						                              getString("cursor_auto_scroll_amount"),
						                              config.cursorPositionThreshold);
						ImGui::Separator();

						addPinnedFloatProperty(quickSettingScrollSpeedNormal,
						                       getString("scroll_speed_normal"),
						                       config.scrollSpeedNormal, "%.1fx");
						addPinnedFloatProperty(quickSettingScrollSpeedShift,
						                       getString("scroll_speed_shift"),
						                       config.scrollSpeedShift, "%.1fx");
						config.scrollSpeedNormal = std::max(0.1f, config.scrollSpeedNormal);
						config.scrollSpeedShift = std::max(0.1f, config.scrollSpeedShift);
						ImGui::Separator();

						addPinnedCheckboxProperty(quickSettingUseSmoothScrolling,
						                         getString("use_smooth_scroll"),
						                         config.useSmoothScrolling);
						addPinnedSliderProperty(quickSettingSmoothScrollingTime,
						                        getString("smooth_scroll_time"),
						                        config.smoothScrollingTime, 10.0f, 150.0f,
						                        "%.2fms");
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("audio"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedSelectProperty(quickSettingNotesSe, getString("notes_se"),
						                        config.seProfileIndex,
						                        Audio::soundEffectsProfileNames,
						                        Audio::soundEffectsProfileCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingDrawBackground,
						                         getString("draw_background"),
						                         config.drawBackground);

						std::string backgroundFile = config.backgroundImage;
						int result = addPinnedFileProperty(quickSettingBackgroundImage,
						                                   getString("background_image"),
						                                   backgroundFile);
						if (result == 1)
						{
							config.backgroundImage = backgroundFile;
							isBackgroundChangePending = true;
						}
						else if (result == 2)
						{
							IO::FileDialog fileDialog{};
							fileDialog.title = "Open Image File";
							fileDialog.filters = { IO::imageFilter, IO::allFilter };
							fileDialog.parentWindowHandle = Application::windowState.windowHandle;

							if (fileDialog.openFile() == IO::FileDialogResult::OK)
							{
								config.backgroundImage = fileDialog.outputFilename;
								isBackgroundChangePending = true;
							}
						}

						addPinnedPercentSliderProperty(quickSettingBackgroundBrightness,
						                              getString("background_brightnes"),
						                              config.backgroundBrightness);
						ImGui::Separator();

						addPinnedPercentSliderProperty(quickSettingLaneOpacity,
						                              getString("lanes_opacity"),
						                              config.laneOpacity);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("advanced"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingShowTickInProperties,
						                         getString("show_tick_in_properties"),
						                         config.showTickInProperties);
						UI::endPropertyColumns();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "preview")))
				{
					if (ImGui::CollapsingHeader(getString("general"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingPreviewDrawToolbar,
						                         getString("preview_draw_toolbar"),
						                         config.pvDrawToolbar);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("visuals"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedSliderProperty(quickSettingPreviewNoteSpeed,
						                        getString("notes_speed"), config.pvNoteSpeed, 1, 12,
						                        "%.2f");
						addPinnedPercentSliderProperty(quickSettingPreviewStageCover,
						                              getString("stage_cover"),
						                              config.pvStageCover);
						ImGui::Separator();

						float hold_alpha = config.pvHoldAlpha * 100.f;
						addPinnedSliderProperty(quickSettingPreviewHoldAlpha,
						                        getString("holds_alpha"), hold_alpha, 10, 100,
						                        "%.0f%%");
						config.pvHoldAlpha = hold_alpha / 100.f;
						float guide_alpha = config.pvGuideAlpha * 100.f;
						addPinnedSliderProperty(quickSettingPreviewGuideAlpha,
						                        getString("guides_alpha"), guide_alpha, 10, 100,
						                        "%.0f%%");
						config.pvGuideAlpha = guide_alpha / 100.f;
						ImGui::Separator();

						addPinnedCheckboxProperty(quickSettingPreviewMirrorScore,
						                         getString("mirror_score"), config.pvMirrorScore);
						// UI::addCheckboxProperty(getString("flicks_animation"), config.pvFlickAnimation);
						// UI::addCheckboxProperty(getString("holds_animation"), config.pvHoldAnimation);
						addPinnedCheckboxProperty(quickSettingPreviewSimultaneousLine,
						                         getString("simultaneous_lines"),
						                         config.pvSimultaneousLine);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedCheckboxProperty(quickSettingDrawBackground,
						                         getString("draw_background"),
						                         config.drawBackground);

						std::string backgroundFile = config.backgroundImage;
						int result = addPinnedFileProperty(quickSettingBackgroundImage,
						                                   getString("background_image"),
						                                   backgroundFile);
						if (result == 1)
						{
							config.backgroundImage = backgroundFile;
							isBackgroundChangePending = true;
						}
						else if (result == 2)
						{
							IO::FileDialog fileDialog{};
							fileDialog.title = "Open Image File";
							fileDialog.filters = { IO::imageFilter, IO::allFilter };
							fileDialog.parentWindowHandle = Application::windowState.windowHandle;

							if (fileDialog.openFile() == IO::FileDialogResult::OK)
							{
								config.backgroundImage = fileDialog.outputFilename;
								isBackgroundChangePending = true;
							}
						}

						addPinnedPercentSliderProperty(quickSettingPreviewBackgroundBrightness,
						                              getString("background_brightnes"),
						                              config.pvBackgroundBrightness);
						ImGui::Separator();

						addPinnedPercentSliderProperty(quickSettingPreviewStageOpacity,
						                              getString("stage_opacity"),
						                              config.pvStageOpacity);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("audio"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						addPinnedSelectProperty(quickSettingNotesSe, getString("notes_se"),
						                        config.seProfileIndex,
						                        Audio::soundEffectsProfileNames,
						                        Audio::soundEffectsProfileCount);
						UI::endPropertyColumns();
					}
					
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "key_config")))
				{
					updateKeyConfig(bindings, sizeof(bindings) / sizeof(MultiInputBinding*));
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();

			ImGui::EndChild();
			ImGui::SetCursorPos(confirmBtnPos);
			if (ImGui::Button("OK", ImVec2(100, UI::btnNormal.y - 5)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
		return DialogResult::None;
	}

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
				ImGui::OpenPopup(getString("layer_merge_confirm"));
			}
			if (!canMerge) ImGui::EndDisabled();
			ImGui::SameLine();

			bool canDelete = context.selectedLayer >= 0 && context.score.layers.size() > 1;
			if (!canDelete) ImGui::BeginDisabled();
			if (UI::transparentButton(ICON_FA_TRASH, ImVec2(layersButtonHeight, layersButtonHeight), false))
			{
				ImGui::OpenPopup(getString("layer_delete_confirm"));
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

			if (ImGui::BeginPopupModal(getString("layer_delete_confirm"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
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

			if (ImGui::BeginPopupModal(getString("layer_merge_confirm"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
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
			popupOpened = ImGui::BeginPopupModal("Create Folder", NULL, ImGuiWindowFlags_NoResize);
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
				ImGui::Text("Folder Name");
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
	}
}
