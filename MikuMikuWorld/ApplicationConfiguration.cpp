#include "ApplicationConfiguration.h"
#include "IO.h"
#include "JsonIO.h"
#include <filesystem>
#include <fstream>

using namespace nlohmann;

namespace MikuMikuWorld
{
	ApplicationConfiguration config{};
	constexpr const char* CONFIG_VERSION{ "1.11.0" };

	ApplicationConfiguration::ApplicationConfiguration() : version{ CONFIG_VERSION }
	{
		recentFiles.reserve(maxRecentFilesEntries);
		restoreDefault();
	}

	void ApplicationConfiguration::read(const std::string& filename)
	{
		restoreDefault();
		recentFiles.clear();

		std::wstring wFilename = IO::mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return;

		std::ifstream configFile(wFilename);
		json configJson;
		configFile >> configJson;
		configFile.close();

		version = jsonIO::tryGetValue<std::string>(configJson, "version", version);
		language = jsonIO::tryGetValue<std::string>(configJson, "language", language);
		debugEnabled = jsonIO::tryGetValue<bool>(configJson, "debug", debugEnabled);

		if (jsonIO::keyExists(configJson, "file"))
		{
			minifyOutput =
			    jsonIO::tryGetValue<bool>(configJson["file"], "minify_output", minifyOutput);
			defaultExportFormat =
			    jsonIO::tryGetValue<int>(configJson["file"], "export_format", defaultExportFormat);
		}

		if (jsonIO::keyExists(configJson, "window"))
		{
			const json& window = configJson["window"];

			maximized = jsonIO::tryGetValue<bool>(window, "maximized", maximized);
			vsync = jsonIO::tryGetValue<bool>(window, "vsync", vsync);
			showFPS = jsonIO::tryGetValue<bool>(window, "show_fps", showFPS);

			windowPos = jsonIO::tryGetValue(window, "position", windowPos);
			if (windowPos.x <= 0)
				windowPos.x = 150;
			if (windowPos.y <= 0)
				windowPos.y = 100;

			windowSize = jsonIO::tryGetValue(window, "size", windowSize);
			if (windowSize.x <= 0 || windowSize.y <= 0)
			{
				windowSize.x = 1000;
				windowSize.y = 800;
			}
		}

		if (jsonIO::keyExists(configJson, "timeline"))
		{
			timelineWidth =
			    jsonIO::tryGetValue<int>(configJson["timeline"], "lane_width", timelineWidth);
			notesHeight =
			    jsonIO::tryGetValue<int>(configJson["timeline"], "notes_height", notesHeight);
			matchNotesSizeToTimeline = jsonIO::tryGetValue<bool>(
			    configJson["timeline"], "match_notes_size_to_timeline", matchNotesSizeToTimeline);

			division =
			    jsonIO::tryGetValue<int>(configJson["timeline"], "division", division);
			zoom =
			    jsonIO::tryGetValue<float>(configJson["timeline"], "zoom", zoom);
			laneOpacity =
			    jsonIO::tryGetValue<float>(configJson["timeline"], "lane_opacity", laneOpacity);
			backgroundBrightness = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "background_brightness", backgroundBrightness);
			drawBackground =
			    jsonIO::tryGetValue<bool>(configJson["timeline"], "draw_background", drawBackground);
			backgroundImage = jsonIO::tryGetValue<std::string>(
			    configJson["timeline"], "background_image", backgroundImage);

			useSmoothScrolling = jsonIO::tryGetValue<bool>(
			    configJson["timeline"], "smooth_scrolling_enable", useSmoothScrolling);
			smoothScrollingTime = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "smooth_scrolling_time", smoothScrollingTime);
			scrollSpeedNormal = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "scroll_speed_normal", scrollSpeedNormal);
			scrollSpeedShift = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "scroll_speed_fast", scrollSpeedShift);

			drawWaveform =
			    jsonIO::tryGetValue<bool>(configJson["timeline"], "draw_waveform", drawWaveform);

			drawHiSpeedAutomation = jsonIO::tryGetValue<bool>(
			    configJson["timeline"], "draw_hispeed_automation", drawHiSpeedAutomation);
			hiSpeedGraphLimit = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "hispeed_graph_limit", hiSpeedGraphLimit);
			hiSpeedGraphBgOpacity = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "hispeed_graph_bg_opacity", hiSpeedGraphBgOpacity);

			returnToLastSelectedTickOnPause = jsonIO::tryGetValue<bool>(
			    configJson["timeline"], "return_to_last_tick_on_pause",
			    returnToLastSelectedTickOnPause);
			cursorPositionThreshold = jsonIO::tryGetValue<float>(
			    configJson["timeline"], "cursor_position_threshold", cursorPositionThreshold);

			showTickInProperties = jsonIO::tryGetValue<bool>(
			    configJson["timeline"], "show_tick_in_properties", showTickInProperties);
		}

		if (jsonIO::keyExists(configJson, "preview"))
		{
			pvNoteSpeed =
			    jsonIO::tryGetValue<float>(configJson["preview"], "note_speed", pvNoteSpeed);
			pvMirrorScore =
			    jsonIO::tryGetValue<bool>(configJson["preview"], "mirror_score", pvMirrorScore);
			pvDrawToolbar =
			    jsonIO::tryGetValue<bool>(configJson["preview"], "draw_toolbar", pvDrawToolbar);
			pvBackgroundBrightness = jsonIO::tryGetValue<float>(
			    configJson["preview"], "background_brightness", pvBackgroundBrightness);
			pvStageOpacity =
			    jsonIO::tryGetValue<float>(configJson["preview"], "stage_opacity", pvStageOpacity);
			pvStageCover =
			    jsonIO::tryGetValue<float>(configJson["preview"], "stage_cover", pvStageCover);
			pvEffectsProfile =
			    jsonIO::tryGetValue<int>(configJson["preview"], "effects_profile", pvEffectsProfile);
			pvFlickAnimation =
			    jsonIO::tryGetValue<bool>(configJson["preview"], "flick_animation", pvFlickAnimation);
			pvHoldAnimation =
			    jsonIO::tryGetValue<bool>(configJson["preview"], "hold_animation", pvHoldAnimation);
			pvSimultaneousLine = jsonIO::tryGetValue<bool>(
			    configJson["preview"], "simultaneous_line", pvSimultaneousLine);
			pvHoldAlpha =
			    jsonIO::tryGetValue<float>(configJson["preview"], "hold_alpha", pvHoldAlpha);
			pvGuideAlpha =
			    jsonIO::tryGetValue<float>(configJson["preview"], "guide_alpha", pvGuideAlpha);
		}

		if (jsonIO::keyExists(configJson, "theme"))
		{
			accentColor =
			    jsonIO::tryGetValue<int>(configJson["theme"], "accent_color", accentColor);
			userColor =
			    jsonIO::tryGetValue(configJson["theme"], "user_color", userColor);
			baseTheme = (BaseTheme)jsonIO::tryGetValue<int>(
			    configJson["theme"], "base_theme", static_cast<int>(baseTheme));
		}

		if (jsonIO::keyExists(configJson, "save"))
		{
			autoSaveEnabled = jsonIO::tryGetValue<bool>(
			    configJson["save"], "auto_save_enabled", autoSaveEnabled);
			autoSaveInterval = jsonIO::tryGetValue<int>(
			    configJson["save"], "auto_save_interval", autoSaveInterval);
			autoSaveMaxCount = jsonIO::tryGetValue<int>(
			    configJson["save"], "auto_save_max_count", autoSaveMaxCount);
		}

		if (jsonIO::keyExists(configJson, "audio"))
		{
			seProfileIndex =
			    jsonIO::tryGetValue<int>(configJson["audio"], "se_profile", seProfileIndex);
			masterVolume = std::clamp(
			    jsonIO::tryGetValue<float>(configJson["audio"], "master_volume", masterVolume),
			    0.0f, 1.0f);
			bgmVolume = std::clamp(
			    jsonIO::tryGetValue<float>(configJson["audio"], "bgm_volume", bgmVolume),
			    0.0f, 1.0f);
			seVolume = std::clamp(
			    jsonIO::tryGetValue<float>(configJson["audio"], "se_volume", seVolume),
			    0.0f, 1.0f);
		}

		if (jsonIO::keyExists(configJson, "input") &&
		    jsonIO::keyExists(configJson["input"], "bindings"))
		{
			for (auto& [key, value] : configJson["input"]["bindings"].items())
			{
				for (int i = 0; i < sizeof(bindings) / sizeof(MultiInputBinding*); ++i)
				{
					if (bindings[i]->name == key)
					{
						int keysCount = std::min(value.size(), bindings[i]->bindings.size());
						for (int k = 0; k < keysCount; ++k)
							bindings[i]->bindings[k] = FromSerializedString(value[k]);

						bindings[i]->count = keysCount;
					}
				}
			}
		}

		if (jsonIO::arrayHasData(configJson, "recent_files"))
		{
			const json& recentFilesJson = configJson["recent_files"];
			const size_t count = std::min(recentFilesJson.size(), maxRecentFilesEntries);
			recentFiles.insert(recentFiles.end(), recentFilesJson.begin(),
			                   recentFilesJson.begin() + count);
		}
	}

	void ApplicationConfiguration::write(const std::string& filename)
	{
		json config;

		// update to latest version
		config["version"] = CONFIG_VERSION;
		config["language"] = language;
		config["debug"] = debugEnabled;
		config["file"]["minify_output"] = minifyOutput;
		config["file"]["export_format"] = defaultExportFormat;
		config["window"]["position"] = { { "x", windowPos.x }, { "y", windowPos.y } };

		config["window"]["size"] = { { "x", windowSize.x }, { "y", windowSize.y } };

		config["window"]["maximized"] = maximized;
		config["window"]["vsync"] = vsync;
		config["window"]["show_fps"] = showFPS;

		config["timeline"] = { { "lane_width", timelineWidth },
			                   { "notes_height", notesHeight },
			                   { "match_notes_size_to_timeline", matchNotesSizeToTimeline },
			                   { "division", division },
			                   { "zoom", zoom },
			                   { "lane_opacity", laneOpacity },
			                   { "background_brightness", backgroundBrightness },
			                   { "draw_background", drawBackground },
			                   { "background_image", backgroundImage },
			                   { "smooth_scrolling_enable", useSmoothScrolling },
			                   { "smooth_scrolling_time", smoothScrollingTime },
			                   { "scroll_speed_normal", scrollSpeedNormal },
			                   { "scroll_speed_fast", scrollSpeedShift },
			                   { "draw_waveform", drawWaveform },
			                   { "draw_hispeed_automation", drawHiSpeedAutomation },
			                   { "hispeed_graph_limit", hiSpeedGraphLimit },
			                   { "hispeed_graph_bg_opacity", hiSpeedGraphBgOpacity },
			                   { "return_to_last_tick_on_pause", returnToLastSelectedTickOnPause },
			                   { "cursor_position_threshold", cursorPositionThreshold },
			                   { "show_tick_in_properties", showTickInProperties } };

		config["preview"] = {
			{ "note_speed", pvNoteSpeed },
			{ "mirror_score", pvMirrorScore },
			{ "draw_toolbar", pvDrawToolbar },
			{ "background_brightness", pvBackgroundBrightness },
			{ "stage_opacity", pvStageOpacity },
			{ "stage_cover", pvStageCover },
			{ "effects_profile", pvEffectsProfile },
			{ "flick_animation", pvFlickAnimation },
			{ "hold_animation", pvHoldAnimation },
			{ "simultaneous_line", pvSimultaneousLine },
			{ "hold_alpha", pvHoldAlpha },
			{ "guide_alpha", pvGuideAlpha }
		};

		config["theme"] = { { "accent_color", accentColor },
			                { "user_color",
			                  { { "r", userColor.r },
			                    { "g", userColor.g },
			                    { "b", userColor.b },
			                    { "a", userColor.a } } },
			                { "base_theme", static_cast<int>(baseTheme) } };

		config["save"] = { { "auto_save_enabled", autoSaveEnabled },
			               { "auto_save_interval", autoSaveInterval },
			               { "auto_save_max_count", autoSaveMaxCount } };

		config["audio"] = { { "se_profile", seProfileIndex },
			                { "master_volume", masterVolume },
			                { "bgm_volume", bgmVolume },
			                { "se_volume", seVolume } };

		json keyBindings;
		for (const auto& binding : bindings)
		{
			json keys;
			for (int k = 0; k < binding->count; ++k)
			{
				if (binding->bindings[k].keyCode != ImGuiKey_None)
					keys.push_back(ToSerializedString(binding->bindings[k]));
			}

			keyBindings[binding->name] = keys;
		}

		config["input"] = { { "bindings", keyBindings } };

		config["recent_files"] = recentFiles;

		std::wstring wFilename = IO::mbToWideStr(filename);
		std::ofstream configFile(wFilename);
		configFile << std::setw(4) << config;
		configFile.flush();
		configFile.close();
	}

	void ApplicationConfiguration::restoreDefault()
	{
		version = CONFIG_VERSION;

		windowPos = Vector2(150, 100);
		windowSize = Vector2(1000, 800);
		maximized = false;
		vsync = true;
		showFPS = false;

		accentColor = 1;
		userColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
		baseTheme = static_cast<BaseTheme>(0);
		language = "auto";

		minifyOutput = true;
		defaultExportFormat = -1;

		timelineWidth = 26;
		notesHeight = 26;
		matchNotesSizeToTimeline = true;
		division = 8;
		zoom = 2.0f;
		laneOpacity = 0.6f;
		backgroundBrightness = 0.5f;
		drawBackground = true;
		backgroundImage = "";
		useSmoothScrolling = true;
		smoothScrollingTime = 40.0f;
		scrollSpeedNormal = 3.0f;
		scrollSpeedShift = 10.0f;
		cursorPositionThreshold = 0.5f;
		drawWaveform = true;
		drawHiSpeedAutomation = true;
		hiSpeedGraphLimit = 5.0f;
		hiSpeedGraphBgOpacity = 0.50f;
		showTickInProperties = true;
		followCursorInPlayback = true;
		returnToLastSelectedTickOnPause = false;

		pvNoteSpeed = 10.0f;
		pvMirrorScore = false;
		pvDrawToolbar = true;
		pvBackgroundBrightness = 0.5f;
		pvStageOpacity = 1.0f;
		pvStageCover = 0.0f;
		pvEffectsProfile = 0;
		pvFlickAnimation = true;
		pvHoldAnimation = true;
		pvSimultaneousLine = true;
		pvHoldAlpha = 0.8f;
		pvGuideAlpha = 0.7f;

		autoSaveEnabled = true;
		autoSaveInterval = 1;
		autoSaveMaxCount = 60;

		seProfileIndex = 0;
		masterVolume = 0.5f;
		bgmVolume = 1.0f;
		seVolume = 1.0f;

		debugEnabled = false;
	}
}