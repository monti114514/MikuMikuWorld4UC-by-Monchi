#include "ScoreEditorWindows.h"
#include "../../Application.h"
#include "../../ApplicationConfiguration.h"
#include "../../Constants.h"
#include "../../File.h"
#include "../../NoteTypes.h"
#include "ScoreContext.h"
#include "../../AudioTrackUtils.h"
#include "../../UI.h"
#include "../../Utilities.h"
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
				UI::addSelectProperty<FlickType>(getString("flick"), edit.flickType,
				                                 flickDirectionKeys,
				                                 arrayLength(flickDirectionKeys));
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


}
