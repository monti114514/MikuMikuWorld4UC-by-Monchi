#pragma once
#include "Constants.h"
#include "Note.h"
#include "Tempo.h"
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace MikuMikuWorld
{
	id_t getNextSkillID();
	id_t getNextHiSpeedID();
	id_t getNextTempoRuntimeID();
	id_t getNextWaypointRuntimeID();
	id_t getNextAudioClipID();

	enum class SkillEffect : uint8_t
	{
		Score,
		Heal,
		Perfect,
		EffectCount
	};

	constexpr const char* skillEffectTypes[]{
		"skill_effect_score", "skill_effect_heal", "skill_effect_perfect"
	};

	struct SkillTrigger
	{
		id_t ID;
		int tick;
		SkillEffect effect = SkillEffect::Score;
		uint8_t level = 1;
	};

	struct Fever
	{
		int startTick;
		int endTick;
	};

	struct Layer
	{
		std::string name;
		float forceNoteSpeed = 0.0f;
		bool hidden = false;

		// Layer folder UI state.
		bool isFolder = false;
		bool inFolder = false;
		bool isCollapsed = false;
	};

	struct Waypoint
	{
		std::string name;
		int tick;
		id_t runtimeId = static_cast<id_t>(-1);
	};
	
	struct HiSpeedChange
	{
		id_t ID;
		int tick;
		float speed;
		int layer = 0;
		float skips = 0;
		HiSpeedEaseType ease;
		bool hideNotes;
	};

	struct ScoreMetadata
	{
		std::string title;
		std::string artist;
		std::string author;
		std::string musicFile;
		std::string jacketFile;
		float musicOffset;

		int laneExtension = 0;
		int baseLifePoint = 1000;
	};

	struct AudioClip
	{
		id_t ID = static_cast<id_t>(-1);
		std::string sourceFile;
		float sourceStartMs = 0.0f;
		float sourceEndMs = -1.0f;
		float timelineStartMs = 0.0f;
		float fadeInMs = 0.0f;
		float fadeOutMs = 0.0f;
		float gain = 1.0f;
		bool muted = false;
		bool locked = false;
		bool visible = true;
	};

	struct AudioTrack
	{
		std::string name = "BGM";
		bool muted = false;
		bool locked = false;
		bool visible = true;
		std::vector<AudioClip> clips;
	};

	struct Score
	{
		ScoreMetadata metadata;
		std::unordered_map<id_t, Note> notes;
		std::unordered_map<id_t, HoldNote> holdNotes;
		std::vector<Tempo> tempoChanges;
		std::map<int, TimeSignature> timeSignatures;
		std::unordered_map<id_t, HiSpeedChange> hiSpeedChanges;
		std::unordered_map<id_t, SkillTrigger> skills;
		Fever fever;

		std::vector<Layer> layers{ { Layer{ "default" } } };
		std::vector<Waypoint> waypoints;
		AudioTrack audioTrack;

		Score();
	};
}
