#include "ScoreEditorTimeline.h"

#include "../../Colors.h"
#include "../../Constants.h"
#include "../../IO.h"
#include "../../Tempo.h"
#include "TimelineEventControls.h"
#include "../../UI.h"
#include "../../Utilities.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <climits>
#include <string>
#include <vector>

namespace MikuMikuWorld
{
	namespace
	{
		const char* skillBadgeIcon(SkillEffect effect)
		{
			switch (effect)
			{
			case SkillEffect::Score:
				return ICON_FA_ARROW_UP;
			case SkillEffect::Heal:
				return ICON_FA_HEART;
			case SkillEffect::Perfect:
				return ICON_FA_GEM;
			default:
				return ICON_FA_ARROW_UP;
			}
		}

		ImU32 skillBadgeColor(SkillEffect effect)
		{
			switch (effect)
			{
			case SkillEffect::Score:
				return ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.70f, 1.00f, 1.00f));
			case SkillEffect::Heal:
				return ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.88f, 0.30f, 1.00f));
			case SkillEffect::Perfect:
				return ImGui::ColorConvertFloat4ToU32(ImVec4(0.70f, 0.42f, 1.00f, 1.00f));
			default:
				return skillColor;
			}
		}
	}

	void ScoreEditorTimeline::openMetaEventEditor(ScoreContext& context,
	                                              const SelectedMetaEvent& event)
	{
		switch (event.kind)
		{
		case MetaEventKind::Waypoint:
			for (int index = 0; index < context.score.waypoints.size(); ++index)
			{
				const Waypoint& waypoint = context.score.waypoints[index];
				if (waypoint.runtimeId != event.key)
					continue;
				eventEdit.editId = index;
				eventEdit.editName = waypoint.name;
				eventEdit.type = EventType::Waypoint;
				ImGui::OpenPopup("edit_event");
				return;
			}
			break;
		case MetaEventKind::Bpm:
			for (int index = 0; index < context.score.tempoChanges.size(); ++index)
			{
				const Tempo& tempo = context.score.tempoChanges[index];
				if (tempo.runtimeId != event.key)
					continue;
				eventEdit.editId = index;
				eventEdit.editBpm = tempo.bpm;
				eventEdit.type = EventType::Bpm;
				ImGui::OpenPopup("edit_event");
				return;
			}
			break;
		case MetaEventKind::TimeSignature:
			if (context.score.timeSignatures.find((int)event.key) !=
			    context.score.timeSignatures.end())
			{
				const TimeSignature& ts = context.score.timeSignatures[(int)event.key];
				eventEdit.editId = event.key;
				eventEdit.editTimeSignatureNumerator = ts.numerator;
				eventEdit.editTimeSignatureDenominator = ts.denominator;
				eventEdit.type = EventType::TimeSignature;
				ImGui::OpenPopup("edit_event");
				return;
			}
			break;
		case MetaEventKind::Skill:
			if (context.score.skills.find(event.key) != context.score.skills.end())
			{
				const SkillTrigger& skill = context.score.skills[event.key];
				eventEdit.editId = event.key;
				eventEdit.editSkillEffect = skill.effect;
				eventEdit.editSkillLevel = skill.level;
				eventEdit.type = EventType::Skill;
				ImGui::OpenPopup("edit_event");
				return;
			}
			break;
		case MetaEventKind::FeverStart:
		case MetaEventKind::FeverEnd:
			eventEdit.editId = 0;
			eventEdit.editFeverStartTick = context.score.fever.startTick;
			eventEdit.editFeverEndTick = context.score.fever.endTick;
			eventEdit.type = EventType::Fever;
			ImGui::OpenPopup("edit_event");
			return;
		}
	}

	void ScoreEditorTimeline::updateMetaEventDrag(ScoreContext& context)
	{
		if (!isHoldingMetaEvent)
			return;

		if (ImGui::IsMouseDragging(0, 2.0f))
			isMovingMetaEvent = true;

		int appliedMeasureDelta = 0;
		if (isMovingMetaEvent)
		{
			auto startTickForEvent = [this](const SelectedMetaEvent& event) -> int
			{
				switch (event.kind)
				{
				case MetaEventKind::Waypoint:
					for (const Waypoint& waypoint : metaEventDragStartScore.waypoints)
						if (waypoint.runtimeId == event.key)
							return waypoint.tick;
					break;
				case MetaEventKind::Bpm:
					for (const Tempo& tempo : metaEventDragStartScore.tempoChanges)
						if (tempo.runtimeId == event.key)
							return tempo.tick;
					break;
				case MetaEventKind::TimeSignature:
					if (metaEventDragStartScore.timeSignatures.find((int)event.key) !=
					    metaEventDragStartScore.timeSignatures.end())
						return measureToTicks((int)event.key, TICKS_PER_BEAT,
						                      metaEventDragStartScore.timeSignatures);
					break;
				case MetaEventKind::Skill:
					if (metaEventDragStartScore.skills.find(event.key) !=
					    metaEventDragStartScore.skills.end())
						return metaEventDragStartScore.skills.at(event.key).tick;
					break;
				case MetaEventKind::FeverStart:
					return metaEventDragStartScore.fever.startTick;
				case MetaEventKind::FeverEnd:
					return metaEventDragStartScore.fever.endTick;
				}
				return -1;
			};

			int tickDelta = std::max(0, hoverTick) - metaEventDragStartTick;
			int measureDelta =
			    accumulateMeasures(std::max(0, hoverTick), TICKS_PER_BEAT,
			                       metaEventDragStartScore.timeSignatures) -
			    metaEventDragStartMeasure;

			for (const SelectedMetaEvent& event : metaEventDragSelection)
			{
				const int tick = startTickForEvent(event);
				if (tick >= 0 && tick + tickDelta < 0)
					tickDelta = -tick;
				if ((event.kind == MetaEventKind::Bpm && tick == 0) ||
				    (event.kind == MetaEventKind::TimeSignature && event.key == 0))
				{
					tickDelta = 0;
					measureDelta = 0;
				}
			}

			bool canMove = true;
			for (const SelectedMetaEvent& event : metaEventDragSelection)
			{
				if (event.kind == MetaEventKind::Bpm)
				{
					const Tempo* movingTempo = nullptr;
					for (const Tempo& tempo : metaEventDragStartScore.tempoChanges)
						if (tempo.runtimeId == event.key)
							movingTempo = &tempo;
					if (movingTempo == nullptr)
						continue;
					const int targetTick = movingTempo->tick + tickDelta;
					for (const Tempo& other : metaEventDragStartScore.tempoChanges)
					{
						if (other.runtimeId != event.key && other.tick == targetTick)
						{
							canMove = false;
							break;
						}
					}
				}
				else if (event.kind == MetaEventKind::TimeSignature)
				{
					const int targetMeasure = (int)event.key + measureDelta;
					if (targetMeasure < 0)
						canMove = false;
					for (const auto& [measure, _] : metaEventDragStartScore.timeSignatures)
					{
						if (measure != (int)event.key && measure == targetMeasure)
						{
							canMove = false;
							break;
						}
					}
				}
				if (!canMove)
					break;
			}
			if (!canMove)
			{
				tickDelta = 0;
				measureDelta = 0;
			}
			appliedMeasureDelta = measureDelta;

			context.score = metaEventDragStartScore;
			const bool moveFeverRange =
			    metaEventDragSelection.find({ MetaEventKind::FeverStart, 0 }) !=
			        metaEventDragSelection.end() &&
			    metaEventDragSelection.find({ MetaEventKind::FeverEnd, 0 }) !=
			        metaEventDragSelection.end();
			if (moveFeverRange)
			{
				context.score.fever.startTick = metaEventDragStartScore.fever.startTick + tickDelta;
				context.score.fever.endTick = metaEventDragStartScore.fever.endTick + tickDelta;
			}

			for (const SelectedMetaEvent& event : metaEventDragSelection)
			{
				switch (event.kind)
				{
				case MetaEventKind::Waypoint:
					for (Waypoint& waypoint : context.score.waypoints)
						if (waypoint.runtimeId == event.key)
							waypoint.tick += tickDelta;
					break;
				case MetaEventKind::Bpm:
					for (Tempo& tempo : context.score.tempoChanges)
						if (tempo.runtimeId == event.key)
							tempo.tick += tickDelta;
					break;
				case MetaEventKind::TimeSignature:
				{
					const int sourceMeasure = (int)event.key;
					const int targetMeasure = sourceMeasure + measureDelta;
					if (sourceMeasure != targetMeasure &&
					    context.score.timeSignatures.find(sourceMeasure) !=
					        context.score.timeSignatures.end())
					{
						TimeSignature timeSignature =
						    context.score.timeSignatures[sourceMeasure];
						context.score.timeSignatures.erase(sourceMeasure);
						timeSignature.measure = targetMeasure;
						context.score.timeSignatures[targetMeasure] = timeSignature;
					}
					break;
				}
				case MetaEventKind::Skill:
					if (context.score.skills.find(event.key) != context.score.skills.end())
						context.score.skills[event.key].tick += tickDelta;
					break;
				case MetaEventKind::FeverStart:
					if (moveFeverRange)
						break;
					context.score.fever.startTick =
					    std::min(context.score.fever.startTick + tickDelta,
					             context.score.fever.endTick);
					break;
				case MetaEventKind::FeverEnd:
					if (moveFeverRange)
						break;
					context.score.fever.endTick =
					    std::max(context.score.fever.endTick + tickDelta,
					             context.score.fever.startTick);
					break;
				}
			}
		}

		if (ImGui::IsMouseReleased(0))
		{
			if (isMovingMetaEvent)
			{
				if (context.selectedMetaEvents == metaEventDragSelection)
				{
					context.selectedMetaEvents.clear();
					for (const SelectedMetaEvent& event : metaEventDragSelection)
					{
						if (event.kind == MetaEventKind::TimeSignature)
							context.selectedMetaEvents.insert(
							    { event.kind,
							      static_cast<id_t>((int)event.key + appliedMeasureDelta) });
						else
							context.selectedMetaEvents.insert(event);
					}
				}
				context.pushHistory("Move meta events", metaEventDragStartScore, context.score);
			}
			isHoldingMetaEvent = false;
			isMovingMetaEvent = false;
			metaEventDragSelection.clear();
		}
	}

	void ScoreEditorTimeline::drawLeftMetaEventClusters(ScoreContext& context)
	{
		context.assignMissingMetaEventRuntimeIds();
		metaEventRects.clear();
		const float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const float timelineStartX = getTimelineStartX(context);
		const float clusterRight = timelineStartX - (6.0f * dpiScale);
		const float clusterLeftLimit = position.x + (4.0f * dpiScale);
		const float availableWidth = clusterRight - clusterLeftLimit;
		if (availableWidth <= 32.0f * dpiScale)
			return;

		enum class ClusterItemType
		{
			Waypoint,
			Bpm,
			TimeSignature,
			Skill,
		};

		struct ClusterItem
		{
			ClusterItemType type;
			int priority{};
			int tick{};
			float y{};
			float preferredWidth{};
			float minWidth{};
			ImU32 color{};
			std::string id;
			std::string text;
			SelectedMetaEvent selectionKey{};

			int waypointIndex = -1;
			int bpmIndex = -1;
			int timeSignatureMeasure = -1;
			id_t skillId = static_cast<id_t>(-1);
		};

		struct Cluster
		{
			float anchorY{};
			std::vector<ClusterItem> items;
		};

		auto eventY = [this](int tick)
		{
			return position.y - tickToPosition(tick) + visualOffset;
		};

		std::vector<ClusterItem> items;
		for (int index = 0; index < context.score.waypoints.size(); ++index)
		{
			const Waypoint& waypoint = context.score.waypoints[index];
			if (waypoint.tick < 0)
				continue;

			ClusterItem item;
			item.type = ClusterItemType::Waypoint;
			item.priority = 0;
			item.tick = waypoint.tick;
			item.y = eventY(waypoint.tick);
			item.color = waypointColor;
			item.id = IO::formatString("meta_waypoint_%d_%s", waypoint.tick, waypoint.name.c_str());
			item.text = waypoint.name;
			item.selectionKey = { MetaEventKind::Waypoint, waypoint.runtimeId };
			item.preferredWidth = std::min(ImGui::CalcTextSize(item.text.c_str()).x +
			                                   (14.0f * dpiScale),
			                               180.0f * dpiScale);
			item.minWidth = 54.0f * dpiScale;
			item.waypointIndex = index;
			items.push_back(item);
		}

		for (int index = 0; index < context.score.tempoChanges.size(); ++index)
		{
			const Tempo& tempo = context.score.tempoChanges[index];
			ClusterItem item;
			item.type = ClusterItemType::Bpm;
			item.priority = 1;
			item.tick = tempo.tick;
			item.y = eventY(tempo.tick);
			item.color = tempoColor;
			item.id = IO::formatString("meta_bpm_%d_%g", tempo.tick, tempo.bpm);
			item.text = IO::formatString("%g BPM", tempo.bpm);
			item.selectionKey = { MetaEventKind::Bpm, tempo.runtimeId };
			item.preferredWidth = 68.0f * dpiScale;
			item.minWidth = 54.0f * dpiScale;
			item.bpmIndex = index;
			items.push_back(item);
		}

		for (const auto& [measure, ts] : context.score.timeSignatures)
		{
			const int tick = measureToTicks(ts.measure, TICKS_PER_BEAT,
			                                context.score.timeSignatures);
			ClusterItem item;
			item.type = ClusterItemType::TimeSignature;
			item.priority = 2;
			item.tick = tick;
			item.y = eventY(tick);
			item.color = timeColor;
			item.id = IO::formatString("meta_time_signature_%d_%d_%d", tick, ts.numerator,
			                           ts.denominator);
			item.text = IO::formatString("%d/%d", ts.numerator, ts.denominator);
			item.selectionKey = { MetaEventKind::TimeSignature, static_cast<id_t>(measure) };
			item.preferredWidth = 42.0f * dpiScale;
			item.minWidth = 36.0f * dpiScale;
			item.timeSignatureMeasure = measure;
			items.push_back(item);
		}

		for (const auto& [id, skill] : context.score.skills)
		{
			ClusterItem item;
			item.type = ClusterItemType::Skill;
			item.priority = 3;
			item.tick = skill.tick;
			item.y = eventY(skill.tick);
			item.color = skillBadgeColor(skill.effect);
			item.id = IO::formatString("meta_skill_%d_%d", id, skill.tick);
			item.text = IO::formatString("%s Lv.%d", skillBadgeIcon(skill.effect), skill.level);
			item.selectionKey = { MetaEventKind::Skill, id };
			item.preferredWidth = 72.0f * dpiScale;
			item.minWidth = 58.0f * dpiScale;
			item.skillId = id;
			items.push_back(item);
		}

		std::sort(items.begin(), items.end(), [](const ClusterItem& a, const ClusterItem& b) {
			return a.y == b.y ? a.priority < b.priority : a.y < b.y;
		});

		const float clusterThreshold =
		    std::max(ImGui::GetFrameHeightWithSpacing(), 24.0f * dpiScale);
		std::vector<Cluster> clusters;
		for (const ClusterItem& item : items)
		{
			if (clusters.empty() ||
			    std::abs(item.y - clusters.back().anchorY) > clusterThreshold)
			{
				clusters.push_back({ item.y, { item } });
				continue;
			}

			Cluster& cluster = clusters.back();
			cluster.items.push_back(item);
		}

		const float gap = 4.0f * dpiScale;
		for (Cluster& cluster : clusters)
		{
			std::stable_sort(cluster.items.begin(), cluster.items.end(),
			                 [](const ClusterItem& a, const ClusterItem& b) {
				                 return a.priority < b.priority;
			                 });

			std::vector<float> widths;
			widths.reserve(cluster.items.size());
			float totalWidth = gap * std::max(0, static_cast<int>(cluster.items.size()) - 1);
			for (const ClusterItem& item : cluster.items)
			{
				widths.push_back(item.preferredWidth);
				totalWidth += item.preferredWidth;
			}

			if (totalWidth > availableWidth && !widths.empty())
			{
				const float gapWidth = gap * std::max(0, static_cast<int>(widths.size()) - 1);
				const float itemSpace = std::max(1.0f, availableWidth - gapWidth);
				const float preferredSpace = std::max(1.0f, totalWidth - gapWidth);
				const float scale = itemSpace / preferredSpace;
				totalWidth = gapWidth;
				for (size_t i = 0; i < widths.size(); ++i)
				{
					widths[i] = std::max(cluster.items[i].minWidth, widths[i] * scale);
					totalWidth += widths[i];
				}
				if (totalWidth > availableWidth)
				{
					const float uniformWidth =
					    std::max(1.0f, (availableWidth - gapWidth) / widths.size());
					totalWidth = gapWidth;
					for (float& width : widths)
					{
						width = uniformWidth;
						totalWidth += width;
					}
				}
			}

			float x = std::max(clusterLeftLimit, clusterRight - totalWidth);

			for (size_t i = 0; i < cluster.items.size(); ++i)
			{
				const ClusterItem& item = cluster.items[i];
				const bool enabled = item.type == ClusterItemType::Waypoint || !playing;
				ImRect itemRect;
				TimelineEventControls::metaClusterControl(
				    item.id.c_str(), timelineStartX, { x, item.y }, widths[i], item.color,
				    item.text.c_str(), enabled, context.isMetaEventSelected(item.selectionKey),
				    &itemRect);
				metaEventRects[item.selectionKey] = itemRect;
				if (ImGui::IsItemHovered())
					isHoveringNote = true;
				if (enabled && ImGui::IsItemActivated())
				{
					if (ImGui::GetIO().KeyAlt)
						context.selectedMetaEvents.erase(item.selectionKey);
					else if (ImGui::GetIO().KeyCtrl)
						context.toggleMetaEventSelection(item.selectionKey);
					else
						context.selectMetaEvent(item.selectionKey, false);

					if (context.isMetaEventSelected(item.selectionKey))
					{
						isHoldingMetaEvent = true;
						holdingMetaEvent = item.selectionKey;
						metaEventDragSelection = context.selectedMetaEvents;
						metaEventDragStartScore = context.score;
						metaEventDragStartTick = item.tick;
						metaEventDragStartMeasure =
						    accumulateMeasures(std::max(0, item.tick), TICKS_PER_BEAT,
						                       context.score.timeSignatures);
					}
				}
				if (enabled && ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					if (!context.isMetaEventSelected(item.selectionKey))
						context.selectMetaEvent(item.selectionKey, false);
					suppressTimelineContextMenu = true;
					openMetaEventEditor(context, item.selectionKey);
				}

				x += widths[i] + gap;
			}
		}
	}

	bool ScoreEditorTimeline::bpmControl(const ScoreContext& context, const Tempo& tempo)
	{
		return bpmControl(context, tempo.bpm, tempo.tick, !playing);
	}

	bool ScoreEditorTimeline::bpmControl(const ScoreContext& context, float bpm, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;

		Vector2 pos{ getTimelineStartX(context) - (110.0f * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return TimelineEventControls::eventControl(
		    getTimelineStartX(context), pos, tempoColor,
		    IO::formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditorTimeline::timeSignatureControl(const ScoreContext& context, int numerator, int denominator, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;

		Vector2 pos{ getTimelineStartX(context) - (45.0f * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return TimelineEventControls::eventControl(
		    getTimelineStartX(context), pos, timeColor,
		    IO::formatString("%d/%d", numerator, denominator).c_str(), enabled);
	}

	bool ScoreEditorTimeline::skillControl(const ScoreContext& context, const SkillTrigger& skill)
	{
		return skillControl(context, skill.tick, skill.effect, skill.level, !playing);
	}

	bool ScoreEditorTimeline::skillControl(const ScoreContext& context, int tick, SkillEffect effect,
	                                       int level, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(context) - (85 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return TimelineEventControls::eventControl(
		    getTimelineStartX(context), pos, skillBadgeColor(effect),
		    IO::formatString("%s Lv.%d", skillBadgeIcon(effect), level).c_str(), enabled);
	}

	bool ScoreEditorTimeline::skillControl(const ScoreContext& context, int tick, bool enabled)
	{
		return skillControl(context, tick, SkillEffect::Score, 1, enabled);
	}

	bool ScoreEditorTimeline::feverControl(ScoreContext& context, const Fever& fever)
	{
		return feverRangeControl(context, fever.startTick, fever.endTick, !playing);
	}

	bool ScoreEditorTimeline::feverRangeControl(ScoreContext& context, int startTick,
	                                            int endTick, bool enabled)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (drawList && startTick >= 0 && endTick >= 0)
		{
			const ImRect rect = getFeverDisplayRect(context);
			const float x = floorf((rect.Min.x + rect.Max.x) * 0.5f);
			float y1 = position.y - tickToPosition(startTick) + visualOffset;
			float y2 = position.y - tickToPosition(endTick) + visualOffset;
			if (y1 > y2)
				std::swap(y1, y2);

			y1 = std::clamp(y1, position.y, position.y + size.y);
			y2 = std::clamp(y2, position.y, position.y + size.y);
			drawList->AddLine({ x, y1 }, { x, y2 }, feverColor, 2.0f);
		}

		return feverControl(context, startTick, true, enabled) ||
		       feverControl(context, endTick, false, enabled);
	}

	bool ScoreEditorTimeline::feverControl(ScoreContext& context, int tick, bool start, bool enabled)
	{
		if (tick < 0)
			return false;

		std::string txt = "FEVER";
		txt.append(start ? ICON_FA_CARET_UP : ICON_FA_CARET_DOWN);

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const ImRect rect = getFeverDisplayRect(context);
		Vector2 pos{ rect.Min.x + (2.0f * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		SelectedMetaEvent event{ start ? MetaEventKind::FeverStart : MetaEventKind::FeverEnd, 0 };
		ImRect itemRect;
		TimelineEventControls::eventControl(getTimelineEndX(context), pos, feverColor,
		                                    txt.c_str(), enabled,
		                                    context.isMetaEventSelected(event), &itemRect);
		metaEventRects[event] = itemRect;
		if (ImGui::IsItemHovered())
			isHoveringNote = true;
		if (enabled && ImGui::IsItemActivated())
		{
			if (ImGui::GetIO().KeyAlt)
				context.selectedMetaEvents.erase(event);
			else if (ImGui::GetIO().KeyCtrl)
				context.toggleMetaEventSelection(event);
			else
				context.selectMetaEvent(event, false);

			if (context.isMetaEventSelected(event))
			{
				isHoldingMetaEvent = true;
				holdingMetaEvent = event;
				metaEventDragSelection = context.selectedMetaEvents;
				metaEventDragStartScore = context.score;
				metaEventDragStartTick = tick;
				metaEventDragStartMeasure =
				    accumulateMeasures(std::max(0, tick), TICKS_PER_BEAT,
				                       context.score.timeSignatures);
			}
		}
		if (enabled && ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			if (!context.isMetaEventSelected(event))
				context.selectMetaEvent(event, false);
			suppressTimelineContextMenu = true;
			return true;
		}
		return false;
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context, const HiSpeedChange& hiSpeed)
	{
		return hiSpeedControl(context, hiSpeed.tick, hiSpeed.speed, hiSpeed.layer, hiSpeed.skips,
		                      hiSpeed.ease, hiSpeed.hideNotes,
		                      context.selectedHiSpeedChanges.find(hiSpeed.ID) !=
		                          context.selectedHiSpeedChanges.end());
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context, int tick, float speed, int layer, float skip, HiSpeedEaseType ease, bool hideNotes, bool selected)
	{
		bool showLayerName = !(layer == -1 || context.selectedLayer == layer);
		bool enabled = layer == -1 || context.selectedLayer == layer || context.showAllLayers;
		std::string easeStr = ease == HiSpeedEaseType::Linear ? "^" : "";
		std::string speedStr = IO::formatFixedFloatTrimmed(speed);
		std::string skipStr;
		if (!isClose(skip, 0.0f, FLT_EPSILON * 1000))
			skipStr = IO::formatFixedFloatTrimmed(skip, 7, "%+.*f");
		std::string layerStr;
		if (showLayerName)
		{
			auto& layerName = context.score.layers[layer].name;
			layerStr += " (";
			layerStr += layerName.size() > 12 ? (layerName.substr(0, 12) + "..") : layerName;
			layerStr += ")";
		}
		std::string txt = IO::formatString("%s%sx%s%s", easeStr, speedStr, skipStr, layerStr);
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const float hiSpeedStartX = getHiSpeedDisplayRect(context).Min.x;
		Vector2 pos{ hiSpeedStartX + (enabled ? 55 : 112) * dpiScale,
			         position.y - tickToPosition(tick) + visualOffset };
		auto color = hideNotes ? (enabled ? hideSpeedColor : inactiveHideSpeedColor)
		                       : (enabled ? speedColor : inactiveSpeedColor);

		bool activated = TimelineEventControls::eventControl(
		    hiSpeedStartX, pos,
		    selected ? ImGui::ColorConvertFloat4ToU32(generateHighlightColor(
		                   generateHighlightColor(ImGui::ColorConvertU32ToFloat4(color))))
		             : color,
		    txt.c_str(), enabled);
		if (enabled && ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			suppressTimelineContextMenu = true;
			return true;
		}
		return activated;
	}

	bool ScoreEditorTimeline::waypointControl(const ScoreContext& context, const Waypoint& waypoint)
	{
		return waypointControl(context, waypoint.name, waypoint.tick);
	}

	bool ScoreEditorTimeline::waypointControl(const ScoreContext& context, std::string name, int tick)
	{
		if (tick < 0)
			return false;

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(context) - (176 * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return TimelineEventControls::eventControl(getTimelineStartX(context), pos, waypointColor,
		                                           name.c_str(), true);
	}

	void ScoreEditorTimeline::eventEditor(ScoreContext& context)
	{
		ImGui::SetNextWindowSize(ImVec2(300, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_event"))
		{
			std::string editLabel{ "edit_" };
			editLabel.append(eventTypes[(int)eventEdit.type]);
			const auto fitColumn = [appearing = ImGui::IsWindowAppearing(),
			                        &style = ImGui::GetStyle()](const char* label)
			{
				if (!appearing)
					return label;
				float maxWidth = ImGui::GetColumnWidth(0);
				float nextWidth =
				    ImGui::CalcTextSize(label).x + ImGui::GetCursorPosX() + style.ItemSpacing.x;
				if (nextWidth > maxWidth)
					ImGui::SetColumnWidth(0, nextWidth);
				return label;
			};
			float minimumColumnWidth = 30.f;
			ImGui::Text(getString(editLabel));
			ImGui::Separator();

			if (eventEdit.type == EventType::Bpm)
			{
				if (!isArrayIndexInBounds(eventEdit.editId, context.score.tempoChanges))
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);

				Tempo& tempo = context.score.tempoChanges[eventEdit.editId];
				UI::addFloatProperty(fitColumn(getString("bpm")), eventEdit.editBpm, "%g");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					tempo.bpm = std::clamp(eventEdit.editBpm, MIN_BPM, MAX_BPM);

					context.pushHistory("Change tempo", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first tempo change
				if (tempo.tick != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.tempoChanges.erase(context.score.tempoChanges.begin() +
						                                 eventEdit.editId);
						context.pushHistory("Remove tempo change", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::TimeSignature)
			{
				if (context.score.timeSignatures.find(eventEdit.editId) ==
				    context.score.timeSignatures.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);
				fitColumn(getString("time_signature"));
				if (UI::timeSignatureSelect(eventEdit.editTimeSignatureNumerator,
				                            eventEdit.editTimeSignatureDenominator))
				{
					Score prev = context.score;
					TimeSignature& ts = context.score.timeSignatures[eventEdit.editId];
					ts.numerator = std::clamp(abs(eventEdit.editTimeSignatureNumerator),
					                          MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_NUMERATOR);
					ts.denominator = std::clamp(abs(eventEdit.editTimeSignatureDenominator),
					                            MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_DENOMINATOR);

					context.pushHistory("Change time signature", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first time signature
				if (eventEdit.editId != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.timeSignatures.erase(eventEdit.editId);
						context.pushHistory("Remove time signature", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::HiSpeed)
			{
				if (context.score.hiSpeedChanges.find(eventEdit.editId) ==
				    context.score.hiSpeedChanges.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				bool eventEdited = false;
				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);
				UI::addFloatProperty(fitColumn(getString("hi_speed_speed")), eventEdit.editHiSpeed,
				                     "%g");
				eventEdited |= ImGui::IsItemDeactivatedAfterEdit();
				eventEdited |= UI::addSelectProperty(fitColumn(getString("hi_speed_ease")),
				                                     eventEdit.editHiSpeedEase, hiSpeedEaseNames,
				                                     arrayLength(hiSpeedEaseNames));
				UI::addFloatProperty(fitColumn(getString("hi_speed_skip_beat")),
				                     eventEdit.editHiSpeedSkip,
				                     IO::formatString("%%g %s", getString("beat")).c_str());
				eventEdited |= ImGui::IsItemDeactivatedAfterEdit();
				UI::addCheckboxProperty(fitColumn(getString("hi_speed_hide_notes")),
				                        eventEdit.editHiSpeedHideNote);
				eventEdited |= ImGui::IsItemDeactivatedAfterEdit();
				HiSpeedChange& hiSpeed = context.score.hiSpeedChanges[eventEdit.editId];
				if (eventEdited)
				{
					Score prev = context.score;
					hiSpeed.speed = eventEdit.editHiSpeed;
					hiSpeed.skips = eventEdit.editHiSpeedSkip;
					hiSpeed.ease = eventEdit.editHiSpeedEase;
					hiSpeed.hideNotes = eventEdit.editHiSpeedHideNote;
					context.pushHistory("Change hi-speed", prev, context.score);
				}
				UI::endPropertyColumns();

				if (hiSpeed.tick != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.hiSpeedChanges.erase(eventEdit.editId);
						context.pushHistory("Remove hi-speed change", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::Skill)
			{
				if (context.score.skills.find(eventEdit.editId) == context.score.skills.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				bool eventEdited = false;
				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);
				eventEdited |= UI::addSelectProperty(fitColumn(getString("skill_effect")),
				                                     eventEdit.editSkillEffect, skillEffectTypes,
				                                     arrayLength(skillEffectTypes));
				eventEdited |= UI::addIntProperty(fitColumn(getString("skill_level")),
				                                  eventEdit.editSkillLevel, "Lv.%d", 1, 4);
				UI::endPropertyColumns();

				SkillTrigger& skill = context.score.skills[eventEdit.editId];
				if (eventEdited)
				{
					Score prev = context.score;
					skill.effect = eventEdit.editSkillEffect;
					skill.level = static_cast<uint8_t>(std::clamp(eventEdit.editSkillLevel, 1, 4));
					context.pushHistory("Change skill trigger", prev, context.score);
				}

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.skills.erase(eventEdit.editId);
					context.pushHistory("Remove skill trigger", prev, context.score);
				}
			}
			else if (eventEdit.type == EventType::Fever)
			{
				bool eventEdited = false;
				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);
				eventEdited |= UI::addIntProperty(fitColumn(getString("fever_start_tick")),
				                                  eventEdit.editFeverStartTick, -1, INT_MAX);
				eventEdited |= UI::addIntProperty(fitColumn(getString("fever_end_tick")),
				                                  eventEdit.editFeverEndTick, -1, INT_MAX);
				UI::endPropertyColumns();

				if (eventEdited)
				{
					Score prev = context.score;
					if (eventEdit.editFeverStartTick >= 0 && eventEdit.editFeverEndTick >= 0 &&
					    eventEdit.editFeverEndTick < eventEdit.editFeverStartTick)
					{
						eventEdit.editFeverEndTick = eventEdit.editFeverStartTick;
					}
					context.score.fever.startTick = eventEdit.editFeverStartTick;
					context.score.fever.endTick = eventEdit.editFeverEndTick;
					context.pushHistory("Change FEVER event", prev, context.score);
				}

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.fever.startTick = -1;
					context.score.fever.endTick = -1;
					context.pushHistory("Remove FEVER event", prev, context.score);
				}
			}
			else if (eventEdit.type == EventType::Waypoint)
			{
				if (eventEdit.editId >= context.score.waypoints.size())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				if (ImGui::IsWindowAppearing())
					ImGui::SetColumnWidth(0, minimumColumnWidth);
				UI::addStringProperty(fitColumn(getString("waypoint_name")), eventEdit.editName);
				Waypoint& waypoint = context.score.waypoints[eventEdit.editId];
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					waypoint.name = eventEdit.editName;

					context.pushHistory("Change waypoint", prev, context.score);
				}
				UI::endPropertyColumns();

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.waypoints.erase(context.score.waypoints.begin() +
					                              eventEdit.editId);
					context.pushHistory("Remove waypint", prev, context.score);
				}
			}
			ImGui::EndPopup();
		}
	}
}
