#include "ScoreContext.h"

#include "../../Constants.h"
#include "ScoreMetaEventUtils.h"
#include "../../Tempo.h"

namespace MikuMikuWorld
{
	bool ScoreContext::isMetaEventSelected(const SelectedMetaEvent& event) const
	{
		return selectedMetaEvents.find(event) != selectedMetaEvents.end();
	}

	void ScoreContext::selectMetaEvent(const SelectedMetaEvent& event, bool additive)
	{
		if (!additive)
			clearSelection();
		selectedMetaEvents.insert(event);
	}

	void ScoreContext::toggleMetaEventSelection(const SelectedMetaEvent& event)
	{
		auto it = selectedMetaEvents.find(event);
		if (it == selectedMetaEvents.end())
			selectedMetaEvents.insert(event);
		else
			selectedMetaEvents.erase(it);
	}

	void ScoreContext::assignMissingMetaEventRuntimeIds()
	{
		for (Tempo& tempo : score.tempoChanges)
		{
			if (tempo.runtimeId == static_cast<id_t>(-1))
				tempo.runtimeId = getNextTempoRuntimeID();
		}
		for (Waypoint& waypoint : score.waypoints)
		{
			if (waypoint.runtimeId == static_cast<id_t>(-1))
				waypoint.runtimeId = getNextWaypointRuntimeID();
		}
	}

	int ScoreContext::getMetaEventTick(const SelectedMetaEvent& event) const
	{
		switch (event.kind)
		{
		case MetaEventKind::Waypoint:
			if (const Waypoint* waypoint =
			        ScoreMetaEvents::findWaypointByRuntimeId(score, event.key))
				return waypoint->tick;
			break;
		case MetaEventKind::Bpm:
			if (const Tempo* tempo = ScoreMetaEvents::findTempoByRuntimeId(score, event.key))
				return tempo->tick;
			break;
		case MetaEventKind::TimeSignature:
			if (score.timeSignatures.find((int)event.key) != score.timeSignatures.end())
				return measureToTicks((int)event.key, TICKS_PER_BEAT, score.timeSignatures);
			break;
		case MetaEventKind::Skill:
			if (score.skills.find(event.key) != score.skills.end())
				return score.skills.at(event.key).tick;
			break;
		case MetaEventKind::FeverStart:
		case MetaEventKind::FeverEnd:
			return ScoreMetaEvents::getFeverEndpointTick(score.fever, event.kind);
		}
		return -1;
	}
}
