#include <choc/memory/choc_xxHash.h>
#include "Score.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Constants.h"
#include "File.h"
#include "IO.h"
#include <unordered_set>

using namespace IO;

namespace MikuMikuWorld
{
	id_t nextSkillID = 1;
	id_t nextHiSpeedID = 1;
	id_t nextTempoRuntimeID = 1;
	id_t nextWaypointRuntimeID = 1;
	id_t nextAudioClipID = 1;
	id_t getNextSkillID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextSkillID, sizeof(id_t));
		nextSkillID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 2);
		return nextSkillID;
	}
	id_t getNextHiSpeedID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextHiSpeedID, sizeof(id_t));
		nextHiSpeedID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 3);
		return nextHiSpeedID;
	}

	id_t getNextTempoRuntimeID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextTempoRuntimeID, sizeof(id_t));
		nextTempoRuntimeID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 4);
		return nextTempoRuntimeID;
	}

	id_t getNextWaypointRuntimeID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextWaypointRuntimeID, sizeof(id_t));
		nextWaypointRuntimeID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 5);
		return nextWaypointRuntimeID;
	}

	id_t getNextAudioClipID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextAudioClipID, sizeof(id_t));
		nextAudioClipID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 6);
		return nextAudioClipID;
	}

	Score::Score()
	{
		metadata.title = "";
		metadata.author = "";
		metadata.artist = "";
		metadata.musicOffset = 0;

		tempoChanges.push_back(Tempo());
		timeSignatures[0] = { 0, 4, 4 };
		auto id = getNextHiSpeedID();
		hiSpeedChanges[id] = HiSpeedChange{ id, 0, 1.0f, 0, 0.0f, HiSpeedEaseType::None, false };

		fever.startTick = fever.endTick = -1;
	}
}
