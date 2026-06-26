#pragma once

#include "../../Score.h"
#include "ScoreContext.h"
#include "../../Tempo.h"

namespace MikuMikuWorld::ScoreMetaEvents
{
	const Tempo* findTempoByRuntimeId(const Score& score, id_t runtimeId);
	Tempo* findTempoByRuntimeId(Score& score, id_t runtimeId);

	const Waypoint* findWaypointByRuntimeId(const Score& score, id_t runtimeId);
	Waypoint* findWaypointByRuntimeId(Score& score, id_t runtimeId);

	int getFeverEndpointTick(const Fever& fever, MetaEventKind kind);
	bool isFeverEndpoint(MetaEventKind kind);
}
