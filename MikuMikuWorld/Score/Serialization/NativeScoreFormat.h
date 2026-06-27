#pragma once

namespace MikuMikuWorld::NativeScoreFileFormat
{
	// Version 1: Version + Metadata(title, author, artist, musicFile, musicOffset) + TimeSignature
	// + TempoChange + Note + Hold
	// Version 2: Skill Support, Jacket Support
	// Version 3: Hispeed Support, Store address offset (metadata, events, taps, holds)
	// Version 4: Guide Support, use platform path for metadata
	inline constexpr int MMWS_VERSION = 4;
	inline constexpr const char* MMWS_SIGNATURE = "MMWS";

	// Version 1: Damage note support, Lane extension support
	// Version 2: Support hold fade type, Change version to int16
	// Version 3: Support more guide colors
	// Version 4: Support layers
	// Version 5: Support waypoints
	// Version 6: Use floating type for lane and width
	inline constexpr int CC_MMWS_VERSION = 6;
	inline constexpr const char* CC_MMWS_SIGNATURE = "CCMMWS";

	// Version 1: Revert version to int32, support dummy note, dummy hold
	// Version 2: Support hispeed easing, skip, hides note; down flicks
	// Version 3: Note data structure refactor; skill effects; sound effect; life point; hold layer
	// Version 4: Force note speed
	inline constexpr int UC_MMWS_VERSION = 4;
	inline constexpr const char* UC_MMWS_SIGNATURE = "UCMMWS";

	// Version 1: Monchi native layout based on UC v2 plus life point, extended skill,
	// sound effect, hold step layer, and force note speed.
	inline constexpr int MCH_MMWS_VERSION = 1;
	inline constexpr const char* MCH_MMWS_SIGNATURE = "MCHMMWS";
	inline constexpr int MCH_LAYER_EXTENSION_VERSION = 1;
	inline constexpr const char* MCH_LAYER_EXTENSION_SIGNATURE = "MCH_LAYER_EXT";
	inline constexpr int MCH_AUDIO_EXTENSION_VERSION = 1;
	inline constexpr const char* MCH_AUDIO_EXTENSION_SIGNATURE = "MCH_AUDIO_EXT";

	enum NoteFlags
	{
		NOTE_CRITICAL = 1 << 0,
		NOTE_FRICTION = 1 << 1,
		NOTE_DUMMY = 1 << 2,
	};

	enum HoldFlags
	{
		HOLD_START_HIDDEN = 1 << 0,
		HOLD_END_HIDDEN = 1 << 1,
		HOLD_GUIDE = 1 << 2,
		HOLD_DUMMY = 1 << 3
	};

	enum ExtendedNoteFlags
	{
		EXT_NOTE_CRITICAL = 1 << 0,
		EXT_NOTE_TRACE = 1 << 1,
		EXT_NOTE_DUMMY = 1 << 2,
		EXT_NOTE_ATTACHED = 1 << 3,
		EXT_NOTE_HIDDEN = 1 << 4,
	};

	enum ExtendedHoldStepFlags
	{
		EXT_HOLD_STEP_CRITICAL = 1 << 0,
		EXT_HOLD_STEP_DUMMY = 1 << 1,
		EXT_HOLD_STEP_GUIDE = 1 << 2,
	};

	enum LayerFlags
	{
		LAYER_HIDDEN = 1 << 0,
		LAYER_IS_FOLDER = 1 << 1,
		LAYER_IN_FOLDER = 1 << 2,
		LAYER_IS_COLLAPSED = 1 << 3
	};

	enum AudioTrackFlags
	{
		AUDIO_TRACK_MUTED = 1 << 0,
		AUDIO_TRACK_LOCKED = 1 << 1,
		AUDIO_TRACK_VISIBLE = 1 << 2,
		AUDIO_TRACK_EXPLICIT = 1 << 3
	};

	enum AudioClipFlags
	{
		AUDIO_CLIP_MUTED = 1 << 0,
		AUDIO_CLIP_LOCKED = 1 << 1,
		AUDIO_CLIP_VISIBLE = 1 << 2
	};
}
