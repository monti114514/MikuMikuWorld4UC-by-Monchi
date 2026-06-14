#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class NoteType : uint8_t
	{
		Tap,
		Hold,
		HoldMid,
		HoldEnd,
		Damage,
	};

	enum class FlickType : uint8_t
	{
		None,
		Default,
		Left,
		Right,
		Down,
		DownLeft,
		DownRight,
		FlickTypeCount
	};

	constexpr const char* flickTypes[]{
		"none", "default", "left", "right", "down", "down_left", "down_right"
	};

	enum class SoundEffectType : uint8_t
	{
		Default,
		None,
		TapPerfect,
		Flick,
		Trace,
		Tick,
		CritTap,
		CritFlick,
		CritTrace,
		CritTick,
		Damage,
		SoundEffectTypeCount
	};

	constexpr const char* soundEffectTypes[]{
		"sound_effect_default", "sound_effect_none",       "sound_effect_tap_perfect",
		"sound_effect_flick",   "sound_effect_trace",      "sound_effect_tick",
		"sound_effect_crit_tap", "sound_effect_crit_flick", "sound_effect_crit_trace",
		"sound_effect_crit_tick", "sound_effect_damage"
	};

	enum class HoldStepType : uint8_t
	{
		Normal,
		Hidden,
		Skip,
		HoldStepTypeCount
	};

	constexpr const char* stepTypes[]{ "normal", "hidden", "skip" };

	enum class EaseType : uint8_t
	{
		Linear,
		EaseIn,
		EaseOut,
		EaseInOut,
		EaseOutIn,
		EaseTypeCount
	};

	constexpr const char* easeNames[]{ "linear", "in", "out", "inout", "outin" };

	constexpr const char* easeTypes[]{ "linear", "ease_in", "ease_out", "ease_in_out",
		                               "ease_out_in" };

	enum class HoldNoteType : uint8_t
	{
		Normal,
		Hidden,
		Guide
	};

	constexpr const char* holdTypes[]{ "normal", "hidden", "guide" };

	enum class HoldEndType : uint8_t
	{
		Normal,
		Trace,
		Hidden
	};

	constexpr const char* holdEndTypes[]{ "normal", "trace", "hidden" };

	enum class HoldStepLayer : uint8_t
	{
		Top,
		Bottom,
		Under,
		Over,
		LayerCount
	};

	constexpr const char* holdStepLayers[]{
		"hold_step_layer_top", "hold_step_layer_bottom",
		"hold_step_layer_under", "hold_step_layer_over"
	};

	enum class GuideColor : uint8_t
	{
		Neutral,
		Red,
		Green,
		Blue,
		Yellow,
		Purple,
		Cyan,
		Black,
		GuideColorCount
	};

	constexpr const char* guideColors[]{ "neutral", "red",    "green", "blue",
		                                 "yellow",  "purple", "cyan",  "black" };
	constexpr const char* guideColorsForString[]{ "guide_neutral", "guide_red",    "guide_green",
		                                          "guide_blue",    "guide_yellow", "guide_purple",
		                                          "guide_cyan",    "guide_black" };

	enum class FadeType : uint8_t
	{
		Out,
		None,
		In
	};

	constexpr const char* fadeTypes[]{ "fade_out", "fade_none", "fade_in" };

	enum class HiSpeedEaseType : uint8_t
	{
		None,
		Linear,
		EaseTypeCount
	};

	constexpr const char* hiSpeedEaseNames[] = { "hi_speed_ease_none", "hi_speed_ease_linear" };
}
