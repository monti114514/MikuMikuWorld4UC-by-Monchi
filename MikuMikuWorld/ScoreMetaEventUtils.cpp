#include "ScoreMetaEventUtils.h"

#include <algorithm>

namespace MikuMikuWorld::ScoreMetaEvents
{
	const Tempo* findTempoByRuntimeId(const Score& score, id_t runtimeId)
	{
		auto it = std::find_if(score.tempoChanges.begin(), score.tempoChanges.end(),
		                       [runtimeId](const Tempo& tempo)
		                       { return tempo.runtimeId == runtimeId; });
		return it == score.tempoChanges.end() ? nullptr : &(*it);
	}

	Tempo* findTempoByRuntimeId(Score& score, id_t runtimeId)
	{
		auto it = std::find_if(score.tempoChanges.begin(), score.tempoChanges.end(),
		                       [runtimeId](const Tempo& tempo)
		                       { return tempo.runtimeId == runtimeId; });
		return it == score.tempoChanges.end() ? nullptr : &(*it);
	}

	const Waypoint* findWaypointByRuntimeId(const Score& score, id_t runtimeId)
	{
		auto it = std::find_if(score.waypoints.begin(), score.waypoints.end(),
		                       [runtimeId](const Waypoint& waypoint)
		                       { return waypoint.runtimeId == runtimeId; });
		return it == score.waypoints.end() ? nullptr : &(*it);
	}

	Waypoint* findWaypointByRuntimeId(Score& score, id_t runtimeId)
	{
		auto it = std::find_if(score.waypoints.begin(), score.waypoints.end(),
		                       [runtimeId](const Waypoint& waypoint)
		                       { return waypoint.runtimeId == runtimeId; });
		return it == score.waypoints.end() ? nullptr : &(*it);
	}

	int getFeverEndpointTick(const Fever& fever, MetaEventKind kind)
	{
		return kind == MetaEventKind::FeverStart ? fever.startTick : fever.endTick;
	}

	bool isFeverEndpoint(MetaEventKind kind)
	{
		return kind == MetaEventKind::FeverStart || kind == MetaEventKind::FeverEnd;
	}
}
