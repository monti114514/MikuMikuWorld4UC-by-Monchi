#include "ScoreEditorTimeline.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "Tempo.h"
#include "Score.h"
#include "AudioTrackUtils.h"
#include "IO.h"
#include "UI.h"
#include "Utilities.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace MikuMikuWorld
{
	namespace
	{
		const HiSpeedChange* findActiveLayerHiSpeed(const Score& score, int layer, int tick)
		{
			const HiSpeedChange* active = nullptr;
			for (const auto& [id, hiSpeed] : score.hiSpeedChanges)
			{
				if (hiSpeed.layer != layer || hiSpeed.tick > tick)
					continue;

				if (!active || hiSpeed.tick > active->tick ||
				    (hiSpeed.tick == active->tick && hiSpeed.ID > active->ID))
					active = &hiSpeed;
			}

			return active;
		}

		bool isHideNotesActive(const Score& score, int layer, int tick)
		{
			if (layer < 0 || layer >= static_cast<int>(score.layers.size()))
				return false;

			const HiSpeedChange* active = findActiveLayerHiSpeed(score, layer, tick);
			return active && active->hideNotes;
		}

		std::vector<int> getHiSpeedBoundaryTicks(const Score& score, int layer, int startTick, int endTick)
		{
			std::vector<int> boundaries{ startTick, endTick };
			for (const auto& [id, hiSpeed] : score.hiSpeedChanges)
			{
				if (hiSpeed.layer == layer && hiSpeed.tick > startTick && hiSpeed.tick < endTick)
					boundaries.push_back(hiSpeed.tick);
			}

			std::sort(boundaries.begin(), boundaries.end());
			boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
			return boundaries;
		}

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

	ScoreEditorTimeline* timelineInstance = nullptr;

	void scrollTimeline(ScoreContext& context, const int tick)
	{
		if (timelineInstance != nullptr)
			timelineInstance->scrollTimeline(context, tick);
	}

	bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt, bool enabled,
	                  bool selected = false, ImRect* outRect = nullptr)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);

		ImGui::PushID(pos.y);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 0 });
		bool activated = UI::coloredButton(
		    txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { -1, -1 }, color, enabled);
		ImGui::PopStyleVar();
		ImGui::PopID();
		ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		if (outRect != nullptr)
			*outRect = rect;
		drawList->AddLine({ xPos, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y }, color,
		                  primaryLineThickness);
		if (selected)
			drawList->AddRect(rect.Min, rect.Max, 0xffffffff, 2.0f, ImDrawFlags_RoundCornersAll,
			                  1.5f);

		return activated;
	}

	bool metaClusterControl(const char* id, float lineStartX, Vector2 pos, float width, ImU32 color,
	                        const char* txt, bool enabled, bool selected = false,
	                        ImRect* outRect = nullptr)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);

		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 0 });
		const float buttonWidth = std::max(1.0f, width - ImGui::GetStyle().FramePadding.x);
		bool activated = UI::coloredButton(
		    txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { buttonWidth, -1 },
		    color, enabled);
		ImGui::PopStyleVar();
		ImGui::PopID();
		ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		if (outRect != nullptr)
			*outRect = rect;
		drawList->AddLine({ lineStartX, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y },
		                  color, primaryLineThickness);
		if (selected)
			drawList->AddRect(rect.Min, rect.Max, 0xffffffff, 2.0f, ImDrawFlags_RoundCornersAll,
			                  1.5f);

		return activated;
	}

	float ScoreEditorTimeline::getNoteYPosFromTick(int tick) const
	{
		return position.y + tickToPosition(tick) - visualOffset + size.y;
	}

	int ScoreEditorTimeline::positionToTick(double pos) const
	{
		return roundf(pos / (unitHeight * zoom));
	}

	float ScoreEditorTimeline::tickToPosition(int tick) const { return tick * unitHeight * zoom; }

	float ScoreEditorTimeline::positionToLane(float pos) const
	{
		return (pos - laneOffset) / laneWidth;
	}

	float ScoreEditorTimeline::laneToPosition(float lane) const
	{
		return laneOffset + (lane * laneWidth);
	}

	ImRect ScoreEditorTimeline::getFeverDisplayRect(const ScoreContext& context) const
	{
		const float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const float gap = 8.0f * dpiScale;
		const float width = 78.0f * dpiScale;
		const float x = getTimelineEndX(context) + gap;
		return ImRect({ x, position.y }, { x + width, position.y + size.y });
	}

	ImRect ScoreEditorTimeline::getHiSpeedDisplayRect(const ScoreContext& context) const
	{
		const float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const ImRect feverRect = getFeverDisplayRect(context);
		const float x = feverRect.Max.x + (10.0f * dpiScale);
		const float maxRight = position.x + size.x - (4.0f * dpiScale);
		const float width = std::min(100.0f * dpiScale, std::max(0.0f, maxRight - x));
		return ImRect({ x, position.y }, { x + width, position.y + size.y });
	}

	bool ScoreEditorTimeline::isMouseInFeverDisplayLane(const ScoreContext& context) const
	{
		const ImRect rect = getFeverDisplayRect(context);
		return ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false);
	}

	int ScoreEditorTimeline::getSnapStepTicks() const
	{
		return std::max(1, TICKS_PER_BEAT / (division / 4));
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
				metaClusterControl(item.id.c_str(), timelineStartX, { x, item.y },
				                   widths[i], item.color, item.text.c_str(), enabled,
				                   context.isMetaEventSelected(item.selectionKey), &itemRect);
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
					openMetaEventEditor(context, item.selectionKey);
				}

				x += widths[i] + gap;
			}
		}
	}

	bool ScoreEditorTimeline::isNoteVisible(const Note& note, int offsetTicks) const
	{
		const float y = getNoteYPosFromTick(note.tick + offsetTicks);
		return y >= 0 && y <= size.y + position.y + 100;
	}

	void ScoreEditorTimeline::setZoom(float value)
	{
		int tick = positionToTick(offset - size.y);
		float x1 = position.y - tickToPosition(tick) + offset;

		zoom = std::clamp(value, minZoom, maxZoom);

		// Prevent jittery movement when zooming
		float x2 = position.y - tickToPosition(tick) + offset;
		visualOffset = offset = std::max(offset + x1 - x2, minOffset);
	}

	int ScoreEditorTimeline::snapTickFromPos(double posY) const
	{
		return snapTick(positionToTick(posY), division);
	}

	int ScoreEditorTimeline::laneFromCenterPosition(const ScoreContext& context, int lane,
	                                                int width)
	{
		return std::clamp(lane - (width / 2), MIN_LANE - context.workingData.laneExtension,
		                  MAX_LANE - width + 1 + context.workingData.laneExtension);
	}

	void ScoreEditorTimeline::focusCursor(ScoreContext& context, Direction direction)
	{
		float cursorY = tickToPosition(context.currentTick);
		if (direction == Direction::Down)
		{
			float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
			if (cursorY <= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
		else
		{
			float timelineOffset = size.y * config.cursorPositionThreshold;
			if (cursorY >= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
	}

	void ScoreEditorTimeline::updateScrollbar()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
		float paddingY = 30.0f;
		ImVec2 windowEndTop = ImGui::GetWindowPos() +
		                      ImVec2{ ImGui::GetWindowSize().x - scrollbarWidth - 4, paddingY };

		ImVec2 windowEndBottom =
		    windowEndTop +
		    ImVec2{ scrollbarWidth + 2,
			        ImGui::GetWindowSize().y - (paddingY * 1.3f) - (UI::toolbarBtnSize.y * 2.0f + ImGui::GetStyle().ItemSpacing.y) - 5 };

		// calculate handle height
		float heightRatio = size.y / ((maxOffset - minOffset) * zoom);
		float handleHeight =
		    std::max(20.0f, ((windowEndBottom.y - windowEndTop.y) * heightRatio) + 30.0f);
		float scrollHeight = windowEndBottom.y - windowEndTop.y - handleHeight;

		// calculate handle position
		float currentOffset = offset - minOffset;
		float positionRatio = std::min(1.0f, currentOffset / ((maxOffset * zoom) - minOffset));
		float handlePosition = windowEndBottom.y - (scrollHeight * positionRatio) - handleHeight;

		// make handle slightly narrower than the background
		ImVec2 scrollHandleMin = ImVec2{ windowEndTop.x + 2, handlePosition };
		ImVec2 scrollHandleMax =
		    ImVec2{ windowEndTop.x + scrollbarWidth - 2, handlePosition + handleHeight };

		// handle "button"
		ImGuiCol handleColor = ImGuiCol_ScrollbarGrab;
		ImGui::SetCursorScreenPos(scrollHandleMin);
		ImGui::InvisibleButton("##scroll_handle", ImVec2{ scrollbarWidth, handleHeight });
		if (ImGui::IsItemHovered())
			handleColor = ImGuiCol_ScrollbarGrabHovered;

		if (ImGui::IsItemActivated())
			scrollStartY = ImGui::GetMousePos().y;

		if (ImGui::IsItemActive())
		{
			handleColor = ImGuiCol_ScrollbarGrabActive;
			float dragDeltaY = scrollStartY - ImGui::GetMousePos().y;
			if (abs(dragDeltaY) > 0)
			{
				// convert handle position to timeline offset
				handlePosition -= dragDeltaY;
				positionRatio =
				    std::min(1.0f, 1 - ((handlePosition - windowEndTop.y) / scrollHeight));
				float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

				offset = newOffset + minOffset;
				scrollStartY = ImGui::GetMousePos().y;
			}
		}

		if (!ImGui::IsItemActive() && positionRatio >= 0.92f)
			maxOffset += 2000;

		ImGui::SetCursorScreenPos(windowEndTop);
		ImGui::InvisibleButton("##scroll_background",
		                       ImVec2{ scrollbarWidth, scrollHeight + handleHeight },
		                       ImGuiButtonFlags_AllowItemOverlap);
		if (ImGui::IsItemActivated())
		{
			float yPos = std::clamp(ImGui::GetMousePos().y, windowEndTop.y,
			                        windowEndBottom.y - handleHeight);

			// convert handle position to timeline offset
			positionRatio = std::clamp(1 - ((yPos - windowEndTop.y)) / scrollHeight, 0.0f, 1.0f);
			float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

			offset = newOffset + minOffset;
		}

		ImU32 scrollBgColor =
		    ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImU32 scrollHandleColor =
		    ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(handleColor));
		drawList->AddRectFilled(windowEndTop, windowEndBottom, scrollBgColor, 0);
		drawList->AddRectFilled(scrollHandleMin, scrollHandleMax, scrollHandleColor,
		                        ImGui::GetStyle().ScrollbarRounding, ImDrawFlags_RoundCornersAll);
	}

	void ScoreEditorTimeline::calculateMaxOffsetFromScore(const Score& score)
	{
		int maxTick = 0;
		for (const auto& [id, note] : score.notes)
			maxTick = std::max(maxTick, note.tick);

		// Current offset maybe greater than calculated offset from score
		maxOffset = std::max(offset / zoom, (maxTick * unitHeight) + 1000);
	}

	void ScoreEditorTimeline::updateScrollingPosition()
	{
		if (config.useSmoothScrolling)
		{
			float scrollAmount = offset - visualOffset;
			float remainingScroll = abs(scrollAmount);
			float delta =
			    scrollAmount / (config.smoothScrollingTime / (ImGui::GetIO().DeltaTime * 1000));

			visualOffset += std::min(remainingScroll, delta);
			remainingScroll = std::max(0.0f, remainingScroll - abs(delta));
		}
		else
		{
			visualOffset = offset;
		}
	}

	void ScoreEditorTimeline::contextMenu(ScoreContext& context)
	{
		if (suppressTimelineContextMenu || ImGui::IsPopupOpen("audio_clip_context"))
			return;

		if (context.isAudioEditing())
		{
			for (const AudioClip& clip : context.score.audioTrack.clips)
			{
				if (clip.visible && getAudioClipRect(context, clip).Contains(ImGui::GetIO().MousePos))
					return;
			}
			return;
		}

		if (ImGui::BeginPopupContextWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
		{
			if (ImGui::MenuItem(getString("delete"), ToShortcutString(config.input.deleteSelection),
			                    false, !context.selectedNotes.empty()))
				context.deleteSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("cut"), ToShortcutString(config.input.cutSelection),
			                    false, !context.selectedNotes.empty()))
				context.cutSelection();

			if (ImGui::MenuItem(getString("copy"), ToShortcutString(config.input.copySelection),
			                    false, !context.selectedNotes.empty()))
				context.copySelection();

			if (ImGui::MenuItem(getString("paste"), ToShortcutString(config.input.paste)))
				context.paste(false);

			if (ImGui::MenuItem(getString("flip_paste"), ToShortcutString(config.input.flipPaste)))
				context.paste(true);

			if (ImGui::MenuItem(getString("duplicate"), ToShortcutString(config.input.duplicate),
			                    false, !context.selectedNotes.empty()))
				context.duplicateSelection(false);

			if (ImGui::MenuItem(getString("flip_duplicate"),
			                    ToShortcutString(config.input.flipDuplicate), false,
			                    !context.selectedNotes.empty()))
				context.duplicateSelection(true);

			if (ImGui::MenuItem(getString("flip"), ToShortcutString(config.input.flip), false,
			                    !context.selectedNotes.empty()))
				context.flipSelection();

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("ease_type"), context.selectionHasEase()))
			{
				for (int i = 0; i < arrayLength(easeTypes); ++i)
					if (ImGui::MenuItem(getString(easeTypes[i])))
						context.setEase((EaseType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("step_type"), context.selectionHasStep()))
			{
				for (int i = 0; i < arrayLength(stepTypes); ++i)
					if (ImGui::MenuItem(getString(stepTypes[i])))
						context.setStep((HoldStepType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("flick_type"), context.selectionHasFlickable()))
			{
				for (int i = 0; i < arrayLength(flickTypes); ++i)
					if (ImGui::MenuItem(getString(flickTypes[i])))
						context.setFlick((FlickType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("hold_type"), context.selectionCanChangeHoldType()))
			{
				/*
				 * We we won't show the option to change holds to guides and vice versa
				 * at least for now since it will complicate changing hold types
				 */
				for (int i = 0; i < arrayLength(holdTypes) - 1; i++)
					if (ImGui::MenuItem(getString(holdTypes[i])))
						context.setHoldType((HoldNoteType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("fade_type"), context.selectionCanChangeFadeType()))
			{
				for (int i = 0; i < arrayLength(fadeTypes); ++i)
					if (ImGui::MenuItem(getString(fadeTypes[i])))
						context.setFadeType((FadeType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("guide_color"), context.selectionCanChangeFadeType()))
			{
				for (int i = 0; i < arrayLength(guideColors); ++i)
				{
					char str[32];
					sprintf_s(str, "guide_%s", guideColors[i]);
					if (ImGui::MenuItem(getString(str)))
						context.setGuideColor((GuideColor)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("layer"), context.selectedNotes.size() > 0))
			{
				for (int i = 0; i < context.score.layers.size(); ++i)
					if (ImGui::MenuItem(context.score.layers[i].name.c_str()))
						context.setLayer(i);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			bool canShrink =
			    (context.selectedNotes.size() + context.selectedHiSpeedChanges.size()) >= 1;
			if (ImGui::MenuItem(getString("shrink_up"), NULL, false, canShrink))
				context.shrinkSelection(Direction::Up);

			if (ImGui::MenuItem(getString("shrink_down"), NULL, false, canShrink))
				context.shrinkSelection(Direction::Down);

			if (ImGui::MenuItem(getString("compress_selection"), NULL, false, canShrink))
				context.compressSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("connect_holds"), NULL, false,
			                    context.selectionCanConnect()))
				context.connectHoldsInSelection();

			if (ImGui::MenuItem(getString("split_hold"), NULL, false,
			                    context.selectionHasStep() && context.selectedNotes.size() == 1))
				context.splitHoldInSelection();

			int selectedMidNum = 0;
			int selectedStartNum = 0;
			for (const auto& noteId : context.selectedNotes)
			{
				auto noteType = context.score.notes.at(noteId).getType();
				if (noteType == NoteType::HoldMid)
					selectedMidNum += 1;
				if (noteType == NoteType::Hold)
					selectedStartNum += 1;
			}
			int selectedTickNum = selectedMidNum + selectedStartNum;

			if (ImGui::MenuItem(getString("repeat_hold_mids"), NULL, false,
			                    selectedTickNum >= 3 and selectedStartNum < 2))
				context.repeatMidsInSelection();

			if (ImGui::BeginMenu(getString("hold_to_traces"), context.selectionHasHold()))
			{
				if (ImGui::MenuItem(getString("add_traces_for_hold")))
					context.convertHoldToTraces(division, false);
				if (ImGui::MenuItem(getString("convert_hold_to_traces")))
					context.convertHoldToTraces(division, true);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("lerp_hispeeds"),
			                     context.selectedHiSpeedChanges.size() >= 2))
			{
				for (int i = 0; i < arrayLength(easeTypes); ++i)
				{
					if (ImGui::MenuItem(getString(easeTypes[i])))
					{
						context.lerpHiSpeeds(division, (EaseType)i);
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("convert_guide_hold"), context.selectionHasHold()))
			{
				if (ImGui::MenuItem(getString("convert_guide_to_hold")))
					context.convertGuideToHold();

				if (ImGui::BeginMenu(getString("convert_hold_to_guide"),
				                     context.selectionHasHold()))
				{
					for (int i = 0; i < arrayLength(guideColors); ++i)
					{
						char str[32];
						sprintf_s(str, "guide_%s", guideColors[i]);
						if (ImGui::MenuItem(getString(str)))
							context.convertHoldToGuide((GuideColor)i);
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}

	void ScoreEditorTimeline::update(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		prevSize = size;
		prevPos = position;

		// Make space for the scrollbar and the status bar
		size = ImGui::GetContentRegionAvail() -
		       ImVec2{ ImGui::GetStyle().ScrollbarSize, UI::toolbarBtnSize.y * 2.0f + ImGui::GetStyle().ItemSpacing.y };

		position = ImGui::GetCursorScreenPos();
		boundaries = ImRect(position, position + size);
		mouseInTimeline = ImGui::IsMouseHoveringRect(position, position + size);

		laneOffset = (size.x * 0.5f) - ((NUM_LANES * laneWidth) * 0.5f);
		minOffset = size.y - 50;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(boundaries.Min, boundaries.Max, true);
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);

		if (background.isDirty())
		{
			background.resize({ size.x, size.y });
			background.process(renderer);
		}
		else if (prevSize.x != size.x || prevSize.y != size.y)
		{
			background.resize({ size.x, size.y });
		}

		if (config.drawBackground)
		{
			const float bgWidth = static_cast<float>(background.getWidth());
			const float bgHeight = static_cast<float>(background.getHeight());
			ImVec2 bgPos{ position.x - (abs(bgWidth - size.x) / 2.0f),
				          position.y - (abs(bgHeight - size.y) / 2.0f) };
			drawList->AddImage((ImTextureID)background.getTextureID(), bgPos,
			                   bgPos + ImVec2{ bgWidth, bgHeight });
		}

		// Remember whether the last mouse click was in the timeline or not
		static bool clickedOnTimeline = false;
		if (ImGui::IsMouseClicked(0))
			clickedOnTimeline = mouseInTimeline;

		const bool pasting = context.pasteData.pasting;
		const ImGuiIO& io = ImGui::GetIO();
		// Get mouse position relative to timeline
		mousePos = io.MousePos - position;
		mousePos.y -= offset;
		if (mouseInTimeline && !UI::isAnyPopupOpen())
		{
			if (io.KeyCtrl)
			{
				setZoom(zoom + (io.MouseWheel * 0.1f));
			}
			else
			{
				float scrollAmount = io.MouseWheel * scrollUnit;
				offset += scrollAmount *
				          (io.KeyShift ? config.scrollSpeedShift : config.scrollSpeedNormal);
			}

			if (context.isNotesEditing() && !isHoveringNote && !isHoldingNote &&
			    !isHoldingMetaEvent && !insertingHold && !pasting &&
			    currentMode == TimelineMode::Select)
			{
				// Clicked inside timeline, the current mouse position is the first drag point
				if (ImGui::IsMouseClicked(0))
				{
					dragStart = mousePos;
					if (!io.KeyCtrl && !io.KeyAlt &&
					    !ImGui::IsPopupOpen(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
					{
						context.selectedNotes.clear();
						context.selectedHiSpeedChanges.clear();
						context.selectedMetaEvents.clear();
					}
				}

				// Clicked and dragging inside the timeline
				if (clickedOnTimeline && ImGui::IsMouseDown(0) &&
				    ImGui::IsMouseDragPastThreshold(0, 10.0f))
					dragging = true;
			}
		}

		offset = std::max(offset, minOffset);
		updateScrollingPosition();

		// Selection rectangle
		// Draw selection rectangle after notes are rendered
		if (dragging && ImGui::IsMouseReleased(0) && !pasting && context.isNotesEditing())
		{
			// Calculate drag selection
			float left = std::min(dragStart.x, mousePos.x);
			float right = std::max(dragStart.x, mousePos.x);
			float top = std::min(dragStart.y, mousePos.y);
			float bottom = std::max(dragStart.y, mousePos.y);

			if (!io.KeyAlt && !io.KeyCtrl)
			{
				context.selectedNotes.clear();
				context.selectedHiSpeedChanges.clear();
				context.selectedMetaEvents.clear();
			}

			float yThreshold = (notesHeight * 0.5f) + 2.0f;
			for (const auto& [id, note] : context.score.notes)
			{
				const bool layerHidden = context.score.layers.at(note.layer).hidden;
				if ((layerHidden || note.layer != context.selectedLayer) && !context.showAllLayers)
					continue;
				float x1 = laneToPosition(note.lane);
				float x2 = laneToPosition(note.lane + note.width);
				float y = -tickToPosition(note.tick);

				if (right > x1 && left < x2 &&
				    isWithinRange(y, top - yThreshold, bottom + yThreshold))
				{
					if (io.KeyAlt)
						context.selectedNotes.erase(id);
					else
						context.selectedNotes.insert(id);
				}
			}
			for (const auto& [id, hsc] : context.score.hiSpeedChanges)
			{
				float dpiScale = ImGui::GetMainViewport()->DpiScale;

				float lx, rx;
				if (config.drawHiSpeedAutomation) {
					// グラフ表示（オートメーション）オンの場合の判定エリア
					const ImRect hiSpeedRect = getHiSpeedDisplayRect(context);
					lx = hiSpeedRect.Min.x - position.x;
					rx = hiSpeedRect.Max.x - position.x;
				} else {
					// 従来のテキスト表示（レガシー）の場合の判定エリア
					const ImRect hiSpeedRect = getHiSpeedDisplayRect(context);
					lx = hiSpeedRect.Min.x - position.x + 60.0f * dpiScale;
					rx = lx + 80.0f * dpiScale;
				}

				float y = -tickToPosition(hsc.tick);

				if (right > lx && left < rx &&
				    (hsc.layer == context.selectedLayer || context.showAllLayers) &&
				    isWithinRange(y, top - yThreshold / 2, bottom + yThreshold / 2))
				{
					if (io.KeyAlt)
						context.selectedHiSpeedChanges.erase(id);
					else
						context.selectedHiSpeedChanges.insert(id);
				}
			}
			ImRect selectionRect({ position.x + left, position.y + top + visualOffset },
			                     { position.x + right, position.y + bottom + visualOffset });
			for (const auto& [event, rect] : metaEventRects)
			{
				if (rect.Max.x < selectionRect.Min.x || rect.Min.x > selectionRect.Max.x ||
				    rect.Max.y < selectionRect.Min.y || rect.Min.y > selectionRect.Max.y)
				{
					continue;
				}
				if (io.KeyAlt)
					context.selectedMetaEvents.erase(event);
				else
					context.selectedMetaEvents.insert(event);
			}

			dragging = false;
		}
		else if (dragging && ImGui::IsMouseReleased(0) && context.isAudioEditing())
		{
			dragging = false;
		}

		const float x1 = getTimelineStartX();
		const float x2 = getTimelineEndX();
		const float exX1 = getTimelineStartX(context);
		const float exX2 = getTimelineEndX(context);

		// Draw solid background color
		drawList->AddRectFilled(
		    { exX1, position.y }, { x1, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x0f));
		drawList->AddRectFilled(
		    { x1, position.y }, { x2, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x1f));
		drawList->AddRectFilled(
		    { x2, position.y }, { exX2, position.y + size.y },
		    Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a,
		                     0x0f));

		if ((config.drawWaveform || context.isAudioEditing()) &&
		    !AudioTrackUtils::hasAudioTrackData(context.score))
			drawWaveform(context);
		drawAudioTrack(context);

		// Draw lanes
		for (int l = 0; l <= NUM_LANES; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y),
			                  boldLane ? divColor1 : divColor2,
			                  boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		// Draw measures
		int firstTick = std::max(0, positionToTick(visualOffset - size.y));
		int lastTick = positionToTick(visualOffset);
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT, context.score.timeSignatures);
		firstTick = measureToTicks(measure, TICKS_PER_BEAT, context.score.timeSignatures);

		int tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		int ticksPerMeasure =
		    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
		int beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;
		int subdivision = TICKS_PER_BEAT / (division / 4);

		// Snap to the sub-division before the current measure to prevent the lines from jumping
		// around
		for (int tick = firstTick - (firstTick % subdivision); tick <= lastTick;
		     tick += subdivision)
		{
			const int y = position.y - tickToPosition(tick) + visualOffset;
			int currentMeasure =
			    accumulateMeasures(tick, TICKS_PER_BEAT, context.score.timeSignatures);

			// Time signature changes on current measure
			if (context.score.timeSignatures.find(currentMeasure) !=
			        context.score.timeSignatures.end() &&
			    currentMeasure != tsIndex)
			{
				tsIndex = currentMeasure;
				ticksPerMeasure =
				    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
				beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;

				// snap to sub-division again on time signature change
				tick = measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);
				tick -= tick % subdivision;
			}

			// determine whether the tick is a beat relative to its measure's tick
			int measureTicks =
			    measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);

			ImU32 color;
			ImU32 exColor;
			float thickness;
			if (!((tick - measureTicks) % beatTicks))
			{
				color = measureColor;
				exColor = exMeasureColor;
				thickness = primaryLineThickness;
			}
			else if (division >= 192)
			{
				continue;
			}
			else
			{
				color = divColor2;
				exColor = exDivColor2;
				thickness = secondaryLineThickness;
			}

			drawList->AddLine(ImVec2(exX1, y), ImVec2(x1, y), exColor, thickness);
			drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), color, thickness);
			drawList->AddLine(ImVec2(x2, y), ImVec2(exX2, y), exColor, thickness);
		}

		tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;

		// Overdraw one measure to make sure the measure string is always visible
		for (int tick = firstTick; tick < lastTick + ticksPerMeasure; tick += ticksPerMeasure)
		{
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
			{
				tsIndex = measure;
				ticksPerMeasure =
				    beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
			}

			std::string measureStr = std::to_string(measure);
			const float txtPos =
			    exX1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);
			const int y = position.y - tickToPosition(tick) + visualOffset;

			drawList->AddLine(ImVec2(exX1 - MEASURE_WIDTH, y), ImVec2(exX2 + MEASURE_WIDTH, y),
			                  measureColor, primaryLineThickness);
			drawShadedText(drawList, ImVec2(txtPos, y), 26, measureTxtColor, measureStr.c_str());

			++measure;
		}

		// draw lanes
		for (int l = -context.workingData.laneExtension;
		     l <= NUM_LANES + context.workingData.laneExtension; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			const bool outOfBounds = l < 0 || l > NUM_LANES;
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y),
			                  outOfBounds ? boldLane ? exDivColor1 : exDivColor2
			                  : boldLane  ? divColor1
			                              : divColor2,
			                  boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		hoverTick = snapTickFromPos(-mousePos.y);
		hoverLane = positionToLane(mousePos.x);
		hoveringNote = -1;
		isHoveringNote = false;
		updateAudioTrackEditing(context);

		// Draw cursor behind notes
		const float y = position.y - tickToPosition(context.currentTick) + visualOffset;
		const int triPtOffset = 6;
		const int triXPos = exX1 - (triPtOffset * 2);
		drawList->AddTriangleFilled(ImVec2(triXPos, y - triPtOffset),
		                            ImVec2(triXPos, y + triPtOffset),
		                            ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(exX1, y), ImVec2(exX2, y), cursorColor,
		                  primaryLineThickness + 1.0f);

		// HSレーン上での右クリックメニュー抑制はいったんWIPとして保留
		contextMenu(context);
		if (config.drawHiSpeedAutomation) {
			// WIP 
			// HSレーン上での右クリックメニュー抑制
			drawHiSpeedGraph(context);
		}

		else
		{
			// 設定がオフの場合は従来のテキスト表示（レガシー）を描画
			for (auto& [id, hiSpeed] : context.score.hiSpeedChanges)
			{
				if (context.score.layers[hiSpeed.layer].hidden &&
				    context.selectedLayer != hiSpeed.layer)
					continue;
				if (hiSpeedControl(context, hiSpeed))
				{
					eventEdit.editId = id;
					eventEdit.editHiSpeed = hiSpeed.speed;
					eventEdit.editHiSpeedEase = hiSpeed.ease;
					eventEdit.editHiSpeedSkip = hiSpeed.skips;
					eventEdit.editHiSpeedHideNote = hiSpeed.hideNotes;
					eventEdit.type = EventType::HiSpeed;
					ImGui::OpenPopup("edit_event");
				}
			}
		}

		drawLeftMetaEventClusters(context);
		updateMetaEventDrag(context);

		if (feverControl(context, context.score.fever))
		{
			eventEdit.editId = 0;
			eventEdit.editFeverStartTick = context.score.fever.startTick;
			eventEdit.editFeverEndTick = context.score.fever.endTick;
			eventEdit.type = EventType::Fever;
			ImGui::OpenPopup("edit_event");
		}

		eventEditor(context);
		updateNotes(context, edit, renderer);

		// Update cursor tick after determining whether a note is hovered
		// The cursor tick should not change if a note is hovered
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && mouseInTimeline && !playing &&
		    !pasting && !UI::isAnyPopupOpen() && currentMode == TimelineMode::Select &&
		    ImGui::IsWindowFocused())
		{
			context.currentTick = hoverTick;
			lastSelectedTick = context.currentTick;
		}

		// Selection boxes
		if (context.isNotesEditing())
		{
			for (id_t id : context.selectedNotes)
			{
				const Note& note = context.score.notes.at(id);
				if (!isNoteVisible(note, 0))
					continue;

				float x = position.x;
				float y = position.y - tickToPosition(note.tick) + visualOffset;

				ImVec2 p1{ x + laneToPosition(note.lane) - 3, y - (notesHeight * 0.5f) };
				ImVec2 p2{ x + laneToPosition(note.lane + note.width) + 3, y + (notesHeight * 0.5f) };

				drawList->AddRectFilled(p1, p2, 0x20f4f4f4, 2.0f, ImDrawFlags_RoundCornersAll);
				drawList->AddRect(p1, p2, 0xcccccccc, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
			}
		}

		if (dragging && !pasting && context.isNotesEditing())
		{
			float startX = std::min(position.x + dragStart.x, position.x + mousePos.x);
			float endX = std::max(position.x + dragStart.x, position.x + mousePos.x);
			float startY =
			    std::min(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			float endY = std::max(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			ImVec2 start{ startX, startY };
			ImVec2 end{ endX, endY };

			drawList->AddRectFilled(start, end, selectionColor1);
			drawList->AddRect(start, end, 0xbbcccccc, 0.2f, ImDrawFlags_RoundCornersAll, 1.0f);

			ImVec2 iconPos = ImVec2(position + dragStart);
			iconPos.y += visualOffset;
			if (io.KeyCtrl)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_PLUS_CIRCLE);
			}
			else if (io.KeyAlt)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_MINUS_CIRCLE);
			}
		}

		drawList->PopClipRect();

		// Status bar: division, snap, edit target, current time and rhythm
		ImGui::SetCursorPos(
		    ImVec2{ ImGui::GetStyle().WindowPadding.x,
		            size.y + UI::toolbarBtnSize.y + 4 + ImGui::GetStyle().WindowPadding.y });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		ImGui::TextUnformatted(getString("quantize"));
		ImGui::SameLine();
		UI::divisionSelect(getString("division"), division, divisions,
		                   sizeof(divisions) / sizeof(int));

		ImGui::SameLine(0.0f, 18.0f);
		ImGui::TextUnformatted(getString("snap_mode"));
		ImGui::SameLine();
		int snapModeInt = (uint8_t)snapMode;
		ImGui::SetNextItemWidth(125);
		if (UI::inlineSelect(getString("snap_mode"), snapModeInt, snapModes,
		                     (size_t)SnapMode::SnapModeMax))
		{
			snapMode = (SnapMode)snapModeInt;
		}

		auto editTargetButton = [&](TimelineEditTarget target, const char* labelKey)
		{
			const bool selected = context.timelineEditTarget == target;
			ImGui::PushStyleColor(ImGuiCol_Button,
			                      selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
			                               : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
			                      selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
			                               : ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,
			                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			// ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

			const char* label = getString(labelKey);
			const float width =
			    std::max(68.0f, ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f);
			if (ImGui::Button(label, ImVec2(width, UI::btnSmall.y)))
			{
				if (context.timelineEditTarget != target)
				{
					context.clearSelection();
					context.timelineEditTarget = target;
					audioRangeClip = static_cast<id_t>(-1);
					audioDragMode = AudioDragMode::None;
				}
			}

			//ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
			//ImGui::ShowStyleEditor();
		};

		ImGui::SameLine(0.0f, 18.0f);
		ImGui::TextUnformatted(getString("edit_target"));
		ImGui::SameLine();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
		                    ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));
		editTargetButton(TimelineEditTarget::Notes, "edit_target_notes");
		ImGui::SameLine();
		editTargetButton(TimelineEditTarget::Audio, "edit_target_audio");
		ImGui::PopStyleVar();

		ImGui::PopStyleColor();

		// ここで改行するために SameLine と Separator を削除します
		
		int currentMeasure =
		    accumulateMeasures(context.currentTick, TICKS_PER_BEAT, context.score.timeSignatures);

		const TimeSignature& ts =
		    context.score
		        .timeSignatures[findTimeSignature(currentMeasure, context.score.timeSignatures)];
		const Tempo& tempo = getTempoAt(context.currentTick, context.score.tempoChanges);
		id_t hiSpeed = findHighSpeedChange(context.currentTick, context.score.hiSpeedChanges,
		                                   context.selectedLayer);
		float speed = (hiSpeed == -1 ? 1.0f : context.score.hiSpeedChanges[hiSpeed].speed);

		std::string rhythmString;
		if (config.showTickInProperties)
		{
			rhythmString = IO::formatString("  %02d:%02d:%02d  |  %dt  |  %d/%d  |  %g BPM  |  %gx",
			                                (int)time / 60, (int)time % 60,
			                                (int)((time - (int)time) * 100), context.currentTick,
			                                ts.numerator, ts.denominator, tempo.bpm, speed);
		}
		else
		{
			rhythmString = IO::formatString(
			    "  %02d:%02d:%02d  |  %d/%d  |  %g BPM  |  %gx", (int)time / 60, (int)time % 60,
			    (int)((time - (int)time) * 100), ts.numerator, ts.denominator, tempo.bpm, speed);
		}

		float _zoom = zoom;
		int controlWidth = ImGui::GetContentRegionAvail().x -
		                   ImGui::CalcTextSize(rhythmString.c_str()).x - (UI::btnSmall.x * 3);
		if (UI::zoomControl("zoom", _zoom, minZoom, 10, std::clamp(controlWidth, 120, 320)))
			setZoom(_zoom);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::Text("%s", rhythmString.c_str());

		updateScrollbar();

		updateNoteSE(context);

		timeLastFrame = time;
		if (playing)
		{
			time += ImGui::GetIO().DeltaTime * playbackSpeed;
			context.currentTick = accumulateTicks(time, TICKS_PER_BEAT, context.score.tempoChanges);

			float cursorY = tickToPosition(context.currentTick);
			if (config.followCursorInPlayback)
			{
				float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
				if (cursorY >= offset - timelineOffset)
					visualOffset = offset = cursorY + timelineOffset;
			}
			else if (cursorY > offset)
			{
				visualOffset = offset = cursorY + size.y;
			}
		}
		else
		{
			time =
			    accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		}
	}

	void ScoreEditorTimeline::updateNotes(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		// directxmath dies
		if (size.y < 10 || size.x < 10)
			return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection", camera.getOffCenterOrthographicProjection(
		                                     0, size.x, position.y, position.y + size.y));

		glEnable(GL_FRAMEBUFFER_SRGB);
		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		const bool notesEditable = context.isNotesEditing();
		const Color audioEditReferenceTint = noteTint * Color{ 1.0f, 1.0f, 1.0f, 0.38f };
		const Color audioEditOtherLayerTint = otherLayerTint * Color{ 1.0f, 1.0f, 1.0f, 0.45f };

		minNoteYDistance = INT_MAX;
		for (auto& [id, note] : context.score.notes)
		{
			const bool layerHidden = context.score.layers.at(note.layer).hidden;
			if (!isNoteVisible(note) || (layerHidden && !context.showAllLayers))
				continue;

			if (note.getType() == NoteType::Tap)
			{
				if (notesEditable)
					updateNote(context, edit, note);
				drawNote(note, renderer,
				         notesEditable
				             ? ((context.showAllLayers || note.layer == context.selectedLayer)
				                    ? noteTint
				                    : otherLayerTint)
				             : ((context.showAllLayers || note.layer == context.selectedLayer)
				                    ? audioEditReferenceTint
				                    : audioEditOtherLayerTint),
				         0, 0, context.showAllLayers || note.layer == context.selectedLayer);
			}
			if (note.getType() == NoteType::Damage)
			{
				if (notesEditable)
					updateNote(context, edit, note);
				drawCcNote(note, renderer,
				           notesEditable
				               ? ((context.showAllLayers || note.layer == context.selectedLayer)
				                      ? noteTint
				                      : otherLayerTint)
				               : ((context.showAllLayers || note.layer == context.selectedLayer)
				                      ? audioEditReferenceTint
				                      : audioEditOtherLayerTint),
				           0, 0, context.showAllLayers || note.layer == context.selectedLayer);
			}
		}

		for (auto& [id, hold] : context.score.holdNotes)
		{
			Note& start = context.score.notes.at(hold.start.ID);
			Note& end = context.score.notes.at(hold.end);

			const bool startLayerHidden = context.score.layers.at(start.layer).hidden;
			const bool endLayerHidden = context.score.layers.at(end.layer).hidden;
			if ((startLayerHidden || endLayerHidden) && !context.showAllLayers)
				continue;

			if (notesEditable && isNoteVisible(start))
				updateNote(context, edit, start);
			if (notesEditable && isNoteVisible(end))
				updateNote(context, edit, end);

			for (const auto& step : hold.steps)
			{
				Note& mid = context.score.notes.at(step.ID);
				if (notesEditable && isNoteVisible(mid))
					updateNote(context, edit, mid);
				if (skipUpdateAfterSortingSteps)
					break;
			}

			drawHoldNote(context.score.notes, hold, renderer,
			             notesEditable ? noteTint : audioEditReferenceTint,
			             context.showAllLayers ? -1 : context.selectedLayer);
		}
		skipUpdateAfterSortingSteps = false;

		renderer->endBatch();
		renderer->beginBatch();

		const bool pasting = context.pasteData.pasting;
		if (notesEditable && pasting && mouseInTimeline && !playing)
		{
			context.pasteData.offsetTicks = hoverTick;
			context.pasteData.offsetLane = hoverLane;
			previewPaste(context, renderer);
			if (ImGui::IsMouseClicked(0))
				context.confirmPaste();
			else if (ImGui::IsMouseClicked(1))
				context.cancelPaste();
		}

		const bool handlingFeverInput =
		    currentMode == TimelineMode::InsertFever && (mouseInTimeline || insertingFever);
		const bool metaInputAllowedInAudioEdit =
		    context.isAudioEditing() &&
		    (currentMode == TimelineMode::InsertBPM ||
		     currentMode == TimelineMode::InsertTimeSign ||
		     currentMode == TimelineMode::InsertSkill ||
		     currentMode == TimelineMode::InsertFever);
		const bool timelineInputAllowed = notesEditable || metaInputAllowedInAudioEdit;
		if (timelineInputAllowed && (mouseInTimeline || handlingFeverInput) && !isHoldingNote &&
		    currentMode != TimelineMode::Select && !pasting && !playing && !UI::isAnyPopupOpen())
		{
			if (currentMode == TimelineMode::InsertFever)
			{
				updateFeverInsertion(context);
			}
			else
			{
				previewInput(context, edit, renderer);
				if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
					executeInput(context, edit);
			}

			if (insertingHold && !ImGui::IsMouseDown(0))
			{
				insertHold(context, edit);
				insertingHold = false;
			}
		}
		else
		{
			insertingHold = false;
			insertingFever = false;
		}

		renderer->endBatch();

		glDisable(GL_FRAMEBUFFER_SRGB);
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddImage((void*)framebuffer->getTexture(), position, position + size);

		// draw hold step outlines
		for (const auto& data : drawSteps)
		{
			const bool layerHidden = context.score.layers.at(data.layer).hidden;
			if (layerHidden && !context.showAllLayers)
				continue;

			drawOutline(data, context.showAllLayers ? -1 : context.selectedLayer);
		}

		drawSteps.clear();
	}

	void ScoreEditorTimeline::previewPaste(ScoreContext& context, Renderer* renderer)
	{
		context.pasteData.offsetLane =
		    std::clamp(hoverLane - context.pasteData.midLane, context.pasteData.minLaneOffset,
		               context.pasteData.maxLaneOffset);

		for (const auto notes : { context.pasteData.notes, context.pasteData.damages })
			for (const auto& [_, note] : notes)
				if (isNoteVisible(note, hoverTick))
				{
					if (note.getType() == NoteType::Tap)
						drawNote(note, renderer, hoverTint, hoverTick, context.pasteData.offsetLane,
						         context.showAllLayers || note.layer == context.selectedLayer);
					else if (note.getType() == NoteType::Damage)
						drawCcNote(note, renderer, hoverTint, hoverTick,
						           context.pasteData.offsetLane,
						           context.showAllLayers || note.layer == context.selectedLayer);
				}

		for (const auto& [_, hold] : context.pasteData.holds)
			drawHoldNote(context.pasteData.notes, hold, renderer, hoverTint, -1, hoverTick,
			             context.pasteData.offsetLane);

		for (const auto& [_, hsc] : context.pasteData.hiSpeedChanges)
			hiSpeedControl(context, hsc.tick + hoverTick, hsc.speed, -1, hsc.skips, hsc.ease,
			               hsc.hideNotes);
	}

	void ScoreEditorTimeline::updateInputNotes(const ScoreContext& context, EditArgs& edit)
	{
		int lane = laneFromCenterPosition(context, hoverLane, edit.noteWidth);
		int width = edit.noteWidth;
		int tick = hoverTick;

		inputNotes.tap.lane = lane;
		inputNotes.tap.width = width;
		inputNotes.tap.tick = tick;
		inputNotes.tap.flick =
		    currentMode == TimelineMode::InsertFlick ? edit.flickType : FlickType::None;
		inputNotes.tap.critical = currentMode == TimelineMode::MakeCritical;
		inputNotes.tap.friction = currentMode == TimelineMode::MakeFriction;
		inputNotes.tap.dummy = currentMode == TimelineMode::MakeDummy;

		inputNotes.holdStep.lane = lane;
		inputNotes.holdStep.width = width;
		inputNotes.holdStep.tick = tick;

		if (insertingHold)
		{
			inputNotes.holdEnd.lane = lane;
			inputNotes.holdEnd.width = width;
			inputNotes.holdEnd.tick = tick;

			if (edit.holdEndType == HoldEndType::Trace)
			{
				inputNotes.holdEnd.friction = true;
			}
			else
			{
				inputNotes.holdEnd.friction = false;
			}
		}
		else
		{
			inputNotes.holdStart.lane = lane;
			inputNotes.holdStart.width = width;
			inputNotes.holdStart.tick = tick;

			if (edit.holdStartType == HoldEndType::Trace)
			{
				inputNotes.holdStart.friction = true;
			}
			else
			{
				inputNotes.holdStart.friction = false;
			}
		}

		inputNotes.damage.lane = lane;
		inputNotes.damage.width = width;
		inputNotes.damage.tick = tick;
	}

	void ScoreEditorTimeline::insertEvent(ScoreContext& context, EditArgs& edit)
	{
		if (currentMode == TimelineMode::InsertBPM)
		{
			for (const auto& tempo : context.score.tempoChanges)
				if (tempo.tick == hoverTick)
					return;

			Score prev = context.score;
			context.score.tempoChanges.push_back({ hoverTick, edit.bpm });
			std::sort(context.score.tempoChanges.begin(), context.score.tempoChanges.end(),
			          [](const auto& a, const auto& b) { return a.tick < b.tick; });
			context.pushHistory("Insert BPM change", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertTimeSign)
		{
			int measure =
			    accumulateMeasures(hoverTick, TICKS_PER_BEAT, context.score.timeSignatures);
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
				return;

			Score prev = context.score;
			context.score.timeSignatures[measure] = { measure, edit.timeSignatureNumerator,
				                                      edit.timeSignatureDenominator };
			context.pushHistory("Insert time signature", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertHiSpeed)
		{
			for (const auto& [_, hs] : context.score.hiSpeedChanges)
				if (hs.tick == hoverTick && hs.layer == context.selectedLayer)
					return;

			Score prev = context.score;
			id_t id = getNextHiSpeedID();
			context.score.hiSpeedChanges[id] = { id,
				                                 hoverTick,
				                                 edit.hiSpeed,
				                                 context.selectedLayer,
				                                 edit.hiSpeedSkip,
				                                 edit.hiSpeedEase,
				                                 edit.hiSpeedHideNotes };
			context.pushHistory("Insert hi-speed changes", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertSkill)
		{
			for (const auto& [_, skill] : context.score.skills)
				if (skill.tick == hoverTick)
					return;

			Score prev = context.score;
			const id_t id = getNextSkillID();
			context.score.skills.emplace(
			    id, SkillTrigger{ id, hoverTick, SkillEffect::Score, static_cast<uint8_t>(1) });
			context.pushHistory("Insert skill", prev, context.score);
		}
	}

	void ScoreEditorTimeline::previewInput(const ScoreContext& context, EditArgs& edit,
	                                       Renderer* renderer)
	{
		updateInputNotes(context, edit);
		switch (currentMode)
		{
		case TimelineMode::InsertLong:
			if (insertingHold)
			{
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, edit.easeType, false, false,
				              renderer, hoverTint);
				if (edit.holdStartType == HoldEndType::Hidden)
				{
					auto drawType = inputNotes.holdStart.critical
					                    ? StepDrawType::InvisibleHoldCritical
					                    : StepDrawType::InvisibleHold;
					drawOutline(StepDrawData{ inputNotes.holdStart, drawType });
				}
				else
				{
					drawNote(inputNotes.holdStart, renderer, hoverTint);
				}

				if (edit.holdEndType == HoldEndType::Hidden)
				{
					auto drawType = inputNotes.holdEnd.critical
					                    ? StepDrawType::InvisibleHoldCritical
					                    : StepDrawType::InvisibleHold;
					drawOutline(StepDrawData{ inputNotes.holdEnd, drawType });
				}
				else
				{
					drawNote(inputNotes.holdEnd, renderer, hoverTint);
				}
			}
			else
			{
				float lane = laneFromCenterPosition(context, hoverLane, edit.noteWidth);
				Note fakeHold = Note{ NoteType::Hold, hoverTick, lane, (float)edit.noteWidth };
				drawNote(fakeHold, renderer, hoverTint);
			}
			break;

		case TimelineMode::InsertLongMid:
			drawHoldMid(inputNotes.holdStep, edit.stepType, renderer, hoverTint);
			drawOutline(StepDrawData(inputNotes.holdStep, edit.stepType == HoldStepType::Skip
			                                                  ? StepDrawType::SkipStep
			                                                  : StepDrawType::NormalStep));
			break;

		case TimelineMode::InsertGuide:
		{
			StepDrawType color = static_cast<StepDrawType>(
			    static_cast<int>(StepDrawType::GuideNeutral) + static_cast<int>(edit.colorType));
			if (insertingHold)
			{
				float a1, a2;
				switch (edit.fadeType)
				{
				case FadeType::Out:
					a1 = 1.0f;
					a2 = 0.0f;
					break;
				case FadeType::In:
					a1 = 0.0f;
					a2 = 1.0f;
					break;
				default:
					a1 = 1.0f;
					a2 = 1.0f;
				}
				if (inputNotes.holdStart.tick > inputNotes.holdEnd.tick)
					std::swap(a1, a2);
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, true,
				              false, renderer, noteTint, 1, 0, a1, a2, edit.colorType);
				drawOutline(StepDrawData(inputNotes.holdStart, color));
				drawOutline(StepDrawData(inputNotes.holdEnd, color));
			}
			else
			{
				drawOutline(StepDrawData(inputNotes.holdStart, color));
			}
		}
		break;

		case TimelineMode::InsertBPM:
			bpmControl(context, edit.bpm, hoverTick, false);
			break;

		case TimelineMode::InsertTimeSign:
			timeSignatureControl(context, edit.timeSignatureNumerator,
			                     edit.timeSignatureDenominator, hoverTick, false);
			break;

		case TimelineMode::InsertHiSpeed:
			hiSpeedControl(context, hoverTick, edit.hiSpeed, -1, edit.hiSpeedSkip, edit.hiSpeedEase,
			               edit.hiSpeedHideNotes);
			break;

		case TimelineMode::InsertSkill:
			skillControl(context, hoverTick, SkillEffect::Score, 1, false);
			break;

		case TimelineMode::InsertDamage:
			drawCcNote(inputNotes.damage, renderer, hoverTint);
			break;

		default:
			drawNote(inputNotes.tap, renderer, hoverTint);
			break;
		}
	}

	void ScoreEditorTimeline::previewFeverInput(ScoreContext& context)
	{
		int startTick = insertingFever ? feverDragStartTick : hoverTick;
		int endTick = insertingFever ? feverDragEndTick : hoverTick + getSnapStepTicks();
		if (startTick < 0 || endTick < 0)
			return;

		feverRangeControl(context, std::min(startTick, endTick), std::max(startTick, endTick),
		                  false);
	}

	void ScoreEditorTimeline::updateFeverInsertion(ScoreContext& context)
	{
		const bool inFeverLane = isMouseInFeverDisplayLane(context);
		if (hoverTick >= 0 && (inFeverLane || insertingFever))
			previewFeverInput(context);

		if (!insertingFever)
		{
			if (inFeverLane && ImGui::IsMouseClicked(0) && hoverTick >= 0)
			{
				insertingFever = true;
				feverDragStartTick = hoverTick;
				feverDragEndTick = hoverTick;
			}
			return;
		}

		if (hoverTick >= 0 && ImGui::IsMouseDown(0))
			feverDragEndTick = hoverTick;

		if (ImGui::IsMouseReleased(0))
			commitFeverInsertion(context);
	}

	void ScoreEditorTimeline::commitFeverInsertion(ScoreContext& context)
	{
		if (feverDragStartTick < 0 || feverDragEndTick < 0)
		{
			insertingFever = false;
			feverDragStartTick = -1;
			feverDragEndTick = -1;
			return;
		}

		int startTick = std::min(feverDragStartTick, feverDragEndTick);
		int endTick = std::max(feverDragStartTick, feverDragEndTick);
		if (startTick == endTick)
			endTick = startTick + getSnapStepTicks();

		Score prev = context.score;
		context.score.fever.startTick = startTick;
		context.score.fever.endTick = endTick;
		context.pushHistory("Set FEVER event", prev, context.score);

		insertingFever = false;
		feverDragStartTick = -1;
		feverDragEndTick = -1;
	}

	void ScoreEditorTimeline::executeInput(ScoreContext& context, EditArgs& edit)
	{
		context.showAllLayers = false;
		switch (currentMode)
		{
		case TimelineMode::InsertTap:
		case TimelineMode::MakeCritical:
		case TimelineMode::MakeFriction:
		case TimelineMode::InsertFlick:
		case TimelineMode::MakeDummy:
			insertNote(context, edit);
			break;

		case TimelineMode::InsertLong:
		case TimelineMode::InsertGuide:
			insertingHold = true;
			break;

		case TimelineMode::InsertLongMid:
		{
			int id = findClosestHold(context, hoverLane, hoverTick);
			if (id != -1)
				insertHoldStep(context, edit, id);
		}
		break;

		case TimelineMode::InsertBPM:
		case TimelineMode::InsertTimeSign:
		case TimelineMode::InsertHiSpeed:
		case TimelineMode::InsertSkill:
			insertEvent(context, edit);
			break;

		case TimelineMode::InsertDamage:
			insertDamage(context, edit);
			break;

		default:
			break;
		}
	}

	void ScoreEditorTimeline::changeMode(TimelineMode mode, EditArgs& edit)
	{
		if (currentMode == mode)
		{
			if (mode == TimelineMode::InsertLong)
			{
				edit.easeType = EaseType((static_cast<int>(edit.easeType) + 1) %
				                         static_cast<int>(EaseType::EaseTypeCount));
			}
			if (mode == TimelineMode::InsertLongMid)
			{
				edit.stepType =
				    (HoldStepType)(((int)edit.stepType + 1) % (int)HoldStepType::HoldStepTypeCount);
			}
			else if (mode == TimelineMode::InsertFlick)
			{
				edit.flickType =
				    (FlickType)(((int)edit.flickType + 1) % (int)FlickType::FlickTypeCount);
				if (!(int)edit.flickType)
					edit.flickType = FlickType::Default;
			}
			else if (mode == TimelineMode::InsertGuide)
			{
				edit.colorType =
				    (GuideColor)(((int)edit.colorType + 1) % (int)GuideColor::GuideColorCount);
			}
		}

		currentMode = mode;
		insertingFever = false;
		feverDragStartTick = -1;
		feverDragEndTick = -1;
	}

	int ScoreEditorTimeline::findClosestHold(ScoreContext& context, int lane, int tick)
	{
		float xt = laneToPosition(lane);
		float yt = getNoteYPosFromTick(tick);

		for (const auto& [id, hold] : context.score.holdNotes)
		{
			const Note& start = context.score.notes.at(hold.start.ID);
			const Note& end = context.score.notes.at(hold.end);

			// No need to search holds outside the cursor's reach
			if (start.tick > tick || end.tick < tick)
				continue;

			// Do not take skip steps into account
			int s1{ -1 }, s2{ -1 };
			while (++s2 < hold.steps.size())
			{
				if (hold.steps[s2].type != HoldStepType::Skip)
					break;
			}

			if (isArrayIndexInBounds(s2, hold.steps))
			{
				// Getting here means we found a non-skip step
				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     context.score.notes.at(hold.steps[s2].ID).layer == context.selectedLayer) &&
				    isMouseInHoldPath(start, context.score.notes.at(hold.steps[s2].ID),
				                      hold.start.ease, xt, yt))
					return id;

				s1 = s2;
				while (++s2 < hold.steps.size())
				{
					if (hold.steps[s2].type != HoldStepType::Skip)
					{
						const Note& m1 = context.score.notes.at(hold.steps[s1].ID);
						const Note& m2 = context.score.notes.at(hold.steps[s2].ID);
						if ((context.showAllLayers || m1.layer == context.selectedLayer ||
						     m2.layer == context.selectedLayer) &&
						    isMouseInHoldPath(m1, m2, hold.steps[s1].ease, xt, yt))
							return id;

						s1 = s2;
					}
				}

				id_t nextId = hold.steps[s1].ID;
				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     context.score.notes.at(nextId).layer == context.selectedLayer) &&
				    isMouseInHoldPath(context.score.notes.at(nextId), end, hold.steps[s1].ease, xt,
				                      yt))
					return id;
			}
			else
			{
				// Hold consists of only skip steps or no steps at all

				if ((context.showAllLayers || start.layer == context.selectedLayer ||
				     end.layer == context.selectedLayer) &&
				    isMouseInHoldPath(start, end, hold.start.ease, xt, yt))
					return id;
			}
		}

		return -1;
	}

	bool ScoreEditorTimeline::noteControl(ScoreContext& context, const Note& note,
	                                      const ImVec2& pos, const ImVec2& sz, const char* id,
	                                      ImGuiMouseCursor cursor)
	{
		float minLane = MIN_LANE - context.workingData.laneExtension;
		float maxLane = MAX_LANE + context.workingData.laneExtension;
		// Do not process notes if the cursor is outside of the timeline
		// This fixes ui buttons conflicting with note "buttons"
		if (!mouseInTimeline && !isHoldingNote)
			return false;

		// Do not allow editing notes during playback
		if (playing)
			return false;

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, sz);
		if (mouseInTimeline && ImGui::IsItemHovered() && !dragging)
			ImGui::SetMouseCursor(cursor);

		// Note clicked
		if (ImGui::IsItemActivated())
		{
			prevUpdateScore = context.score;
			ctrlMousePos = mousePos;
			holdLane = hoverLane;
			holdTick = hoverTick;
		}

		// Holding note
		if (ImGui::IsItemActive())
		{
			ImGui::SetMouseCursor(cursor);
			isHoldingNote = true;
			return true;
		}

		// Note released
		if (ImGui::IsItemDeactivated())
		{
			bool noChange = false;
			auto it = context.score.notes.find(holdingNote);
			if (it != context.score.notes.end())
				noChange = noteTransformOrigin.isSame(it->second);

			isHoldingNote = false;
			holdingNote = 0;

			if (!noChange)
			{
				std::unordered_set<int> sortHolds = context.getHoldsFromSelection();
				for (int id : sortHolds)
				{
					HoldNote& hold = context.score.holdNotes.at(id);
					Note& start = context.score.notes.at(id);
					Note& end = context.score.notes.at(hold.end);

					if (start.tick > end.tick)
					{
						std::swap(start.tick, end.tick);
						std::swap(start.lane, end.lane);
						std::swap(start.width, end.width);
					}

					if (hold.steps.size())
					{
						sortHoldSteps(context.score, hold);

						// Ensure hold steps are between the start and end
						Note& firstMid = context.score.notes.at(hold.steps[0].ID);
						if (start.tick > firstMid.tick)
						{
							std::swap(start.tick, firstMid.tick);
							std::swap(start.lane, firstMid.lane);
							start.lane = std::clamp(start.lane, minLane, maxLane - start.width + 1);
							firstMid.lane =
							    std::clamp(firstMid.lane, minLane, maxLane - firstMid.width + 1);
						}

						Note& lastMid =
						    context.score.notes.at(hold.steps[hold.steps.size() - 1].ID);
						if (end.tick < lastMid.tick)
						{
							std::swap(end.tick, lastMid.tick);
							std::swap(end.lane, lastMid.lane);
							lastMid.lane =
							    std::clamp(lastMid.lane, minLane, maxLane - lastMid.width + 1);
							end.lane = std::clamp(end.lane, minLane, maxLane - end.width + 1);
						}
					}

					sortHoldSteps(context.score, hold);
					skipUpdateAfterSortingSteps = true;
				}

				context.pushHistory("Update notes", prevUpdateScore, context.score);
			}
		}

		return false;
	}

	void ScoreEditorTimeline::updateNote(ScoreContext& context, EditArgs& edit, Note& note)
	{
		if (!(context.showAllLayers || context.selectedLayer == note.layer))
			return;
		const float exLane = context.workingData.laneExtension;
		const float minLane = MIN_LANE - exLane;
		const float maxLane = MAX_LANE + exLane;
		const float maxNoteWidth = MAX_NOTE_WIDTH + exLane * 2;

		const float btnPosY =
		    position.y - tickToPosition(note.tick) + visualOffset - (notesHeight * 0.5f);
		float btnPosX = laneToPosition(note.lane) + position.x - 2.0f;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 noteSz{ laneToPosition(note.lane + note.width) + position.x + 2.0f - btnPosX,
			           notesHeight };
		ImVec2 sz{ noteControlWidth, notesHeight };

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseHoveringRect(pos, pos + noteSz, false) && mouseInTimeline)
		{
			isHoveringNote = true;

			float noteYDistance =
			    std::abs((btnPosY + notesHeight / 2 - visualOffset - position.y) - mousePos.y);
			if (noteYDistance < minNoteYDistance || io.KeyCtrl)
			{
				minNoteYDistance = noteYDistance;
				hoveringNote = note.ID;
				if (ImGui::IsMouseClicked(0) && !UI::isAnyPopupOpen())
				{
					if (!io.KeyCtrl && !io.KeyAlt && !context.isNoteSelected(note))
					{
						context.selectedNotes.clear();
						context.selectedHiSpeedChanges.clear();
						context.selectedMetaEvents.clear();
					}

					context.selectedNotes.insert(note.ID);

					if (io.KeyAlt && context.isNoteSelected(note))
						context.selectedNotes.erase(note.ID);

					if (context.isNoteSelected(note))
					{
						holdingNote = note.ID;
						noteTransformOrigin = NoteTransform::fromNote(note);
					}
				}
			}
		}

		// Left resize
		ImGui::PushID(note.ID);
		if (noteControl(context, note, pos, sz, "L", ImGuiMouseCursor_ResizeEW))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane);
			int diff = curLane - grabLane;

			if (abs(diff) > 0)
			{
				bool canResize = !std::any_of(
				    context.selectedNotes.begin(), context.selectedNotes.end(),
				    [&context, diff, minLane, maxLane, maxNoteWidth](int id)
				    {
					    Note& n = context.score.notes.at(id);
					    int newLane = n.lane + diff;
					    int newWidth = n.width - diff;
					    return (newLane < minLane || newLane + newWidth - 1 > maxLane ||
					            !isWithinRange(newWidth, MIN_NOTE_WIDTH, maxNoteWidth));
				    });

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.width = std::clamp(n.width - diff, (float)MIN_NOTE_WIDTH, maxNoteWidth);
						n.lane = std::clamp(n.lane + diff, minLane, maxLane - n.width + 1);
					}
				}
			}
		}

		pos.x += noteControlWidth;
		// account for <1 width by always having this be positive
		sz.x = std::max((laneWidth * note.width) + 4.0f - (noteControlWidth * 2.0f),
		                (noteControlWidth * 2.0f));

		// Move
		if (noteControl(context, note, pos, sz, "M", ImGuiMouseCursor_Hand))
		{
			float curLane = truncf(positionToLane(mousePos.x));
			float grabLane = truncf(std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane));
			int grabTick = snapTickFromPos(-ctrlMousePos.y);

			int laneDiff = curLane - grabLane;
			if (abs(laneDiff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.x = mousePos.x;
				bool canMove =
				    !std::any_of(context.selectedNotes.begin(), context.selectedNotes.end(),
				                 [&context, laneDiff, minLane, maxLane](int id)
				                 {
					                 Note& n = context.score.notes.at(id);
					                 int newLane = n.lane + laneDiff;
					                 return (newLane < minLane || newLane + n.width - 1 > maxLane);
				                 });

				if (canMove)
				{
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.lane = std::clamp(n.lane + laneDiff, minLane, maxLane - n.width + 1);
					}
				}
			}

			int tickDiff = hoverTick - grabTick;
			if (abs(tickDiff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.y = mousePos.y;

				bool canMove =
				    !std::any_of(context.selectedNotes.begin(), context.selectedNotes.end(),
				                 [&context, tickDiff](int id)
				                 { return context.score.notes.at(id).tick + tickDiff < 0; });

				if (canMove)
				{
					switch (snapMode)
					{
					case SnapMode::Relative:
					{
						for (id_t id : context.selectedNotes)
						{
							Note& n = context.score.notes.at(id);
							n.tick = std::max(n.tick + tickDiff, 0);
						}
						break;
					}
					case SnapMode::Absolute:
					{
						int grabbingNoteTick = note.tick;
						int grabbingNoteTickSnapped =
						    roundTickDown(grabbingNoteTick + tickDiff, division);
						int actualDiff = grabbingNoteTickSnapped - grabbingNoteTick;

						for (id_t id : context.selectedNotes)
						{
							Note& n = context.score.notes.at(id);
							n.tick = std::max(n.tick + actualDiff, 0);
						}

						break;
					}
					case SnapMode::IndividualAbsolute:
					{
						std::vector<id_t> sortedSelectedNotes(context.selectedNotes.size());
						std::copy(context.selectedNotes.begin(), context.selectedNotes.end(),
						          sortedSelectedNotes.begin());
						std::sort(sortedSelectedNotes.begin(), sortedSelectedNotes.end(),
						          [&context](id_t a, id_t b)
						          {
							          return context.score.notes.at(a).tick <
							                 context.score.notes.at(b).tick;
						          });

						for (int id : sortedSelectedNotes)
						{
							Note& n = context.score.notes.at(id);
							auto shiftedTick = n.tick + tickDiff;
							n.tick = std::max(roundTickDown(shiftedTick, division), 0);
						}

						break;
					}
					default:
						throw std::runtime_error("Invalid snap mode (Unreachable)");
					}
				}
			}
		}

		// Per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && !context.selectedNotes.empty())
			{
				switch (currentMode)
				{
				case TimelineMode::InsertFlick:
					context.setFlick(FlickType::FlickTypeCount);
					break;

				case TimelineMode::MakeCritical:
					context.toggleCriticals();
					break;

				case TimelineMode::InsertLong:
					context.setEase(EaseType::EaseTypeCount);
					break;

				case TimelineMode::InsertLongMid:
					context.setStep(HoldStepType::HoldStepTypeCount);
					break;

				case TimelineMode::InsertGuide:
					context.setGuideColor(GuideColor::GuideColorCount);
					break;

				case TimelineMode::MakeFriction:
					context.toggleFriction();
					break;

				case TimelineMode::MakeDummy:
					context.toggleDummy();
					break;

				default:
					break;
				}
			}

			isMovingNote = false;
		}

		pos.x += sz.x;
		sz.x = noteControlWidth;

		// Right resize
		if (noteControl(context, note, pos, sz, "R", ImGuiMouseCursor_ResizeEW))
		{
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane);
			int curLane = positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			if (abs(diff) > 0)
			{
				bool canResize = !std::any_of(
				    context.selectedNotes.begin(), context.selectedNotes.end(),
				    [&context, diff, maxLane](int id)
				    {
					    Note& n = context.score.notes.at(id);
					    int newWidth = n.width + diff;
					    return (newWidth < MIN_NOTE_WIDTH || n.lane + newWidth - 1 > maxLane);
				    });

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					for (id_t id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.width = std::clamp(n.width + diff, (float)MIN_NOTE_WIDTH,
						                     maxNoteWidth - n.lane);
					}
				}
			}
		}

		ImGui::PopID();
	}

	void ScoreEditorTimeline::drawHoldCurve(const Note& n1, const Note& n2, EaseType ease,
	                                        bool isGuide, bool isDummy, Renderer* renderer,
	                                        const Color& tint_, const int offsetTick,
	                                        const int offsetLane, const float startAlpha,
	                                        const float endAlpha, const GuideColor guideColor,
	                                        const int selectedLayer)
	{
		int texIndex{ noteTextures.holdPath };
		ZIndex zIndex{ ZIndex::HoldLine };
		if (isGuide)
		{
			zIndex = ZIndex::Guide;
			texIndex = noteTextures.guideColors;
		}

		if (texIndex == -1)
			return;

		auto tint = tint_;
		const Texture& pathTex = ResourceManager::textures[texIndex];
		const int sprIndex = isGuide ? static_cast<int>(guideColor) : n1.critical ? 3 : 1;
		if (!isArrayIndexInBounds(sprIndex, pathTex.sprites))
			return;

		const Sprite& spr = pathTex.sprites[sprIndex];

		float startX1 = laneToPosition(n1.lane + offsetLane);
		float startX2 = laneToPosition(n1.lane + n1.width + offsetLane);
		float startY = getNoteYPosFromTick(n1.tick + offsetTick);

		float endX1 = laneToPosition(n2.lane + offsetLane);
		float endX2 = laneToPosition(n2.lane + n2.width + offsetLane);
		float endY = getNoteYPosFromTick(n2.tick + offsetTick);

		int left = spr.getX() + holdCutoffX;
		int right = spr.getX() + spr.getWidth() - holdCutoffX;

		auto easeFunc = getEaseFunction(ease);
		float steps = std::max(5.0f, std::ceilf(abs((endY - startY)) / 10));
		for (int y = 0; y < steps; ++y)
		{
			Color inactiveTint = tint * otherLayerTint;
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;

			float xl1 = easeFunc(startX1, endX1, percent1) - 2;
			float xr1 = easeFunc(startX2, endX2, percent1) + 2;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = easeFunc(startX1, endX1, percent2) - 2;
			float xr2 = easeFunc(startX2, endX2, percent2) + 2;

			int z = (selectedLayer == -1 || ((y < steps / 2) ? n1 : n2).layer == selectedLayer)
			            ? (int)ZIndex::zCount
			            : 0;

			if (y2 <= 0)
				continue;

			// rest of hold no longer visible
			if (y1 > size.y + size.y + position.y + 100)
				break;

			Color localTint =
			    selectedLayer == -1
			        ? noteTint
			        : Color::lerp(n1.layer == selectedLayer ? noteTint : inactiveTint,
			                      n2.layer == selectedLayer ? noteTint : inactiveTint, percent1);

			localTint.a = tint.a * lerp(0.7, 1, lerp(startAlpha, endAlpha, percent1));
			localTint.a *= isDummy ? 0.9f : 1.f;

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + holdSliceSize, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + holdSliceSize, y2 };
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left, left + holdSliceWidth, spr.getY(),
			                   spr.getY() + spr.getHeight(), localTint);
			p1.x = xl1 + holdSliceSize;
			p2.x = xr1 - holdSliceSize;
			p3.x = xl2 + holdSliceSize;
			p4.x = xr2 - holdSliceSize;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left + holdSliceWidth,
			                   right - holdSliceWidth, spr.getY(), spr.getY() + spr.getHeight(),
			                   localTint);
			p1.x = xr1 - holdSliceSize;
			p2.x = xr1;
			p3.x = xr2 - holdSliceSize;
			p4.x = xr2;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, right - holdSliceWidth, right, spr.getY(),
			                   spr.getY() + spr.getHeight(), localTint);
		}

		ImDrawList* drawList;
		if (isDummy && (drawList = ImGui::GetWindowDrawList()) && std::abs(endY - startY) > 3)
		{
			const ImU32 outlineCol = IM_COL32(255, 35, 87, 255);
			const float outlineWidth = 6.f;
			const float outlineOffset = -1.f;

			Vector2 topLeft =
			    ImVec2{ startX1 + outlineOffset, size.y - startY + position.y } + position;
			Vector2 topRight =
			    ImVec2{ startX2 - outlineOffset, size.y - startY + position.y } + position;
			Vector2 bottomLeft =
			    ImVec2{ endX1 + outlineOffset, size.y - endY + position.y } + position;
			Vector2 bottomRight =
			    ImVec2{ endX2 - outlineOffset, size.y - endY + position.y } + position;
			auto easeFunc = getEaseFunction(ease);
			Vector2 centerLeft = { midpoint(topLeft.x, bottomLeft.x),
				                   midpoint(topLeft.y, bottomLeft.y) };
			Vector2 centerRight = { midpoint(topRight.x, bottomRight.x),
				                    midpoint(topRight.y, bottomRight.y) };
			auto drawBezier = [&](const Vector2& start, const Vector2& end, EaseType ease)
			{
				auto&& [p1, p2, p3] = convertToBezier(start, end, ease);
				drawList->AddBezierQuadratic(p1, p2, p3, outlineCol, outlineWidth);
			};
			switch (ease)
			{
			case EaseType::EaseIn:
				drawBezier(topLeft, bottomLeft, EaseType::EaseIn);
				drawBezier(topRight, bottomRight, EaseType::EaseIn);
				break;
			case EaseType::EaseOut:
				drawBezier(topLeft, bottomLeft, EaseType::EaseOut);
				drawBezier(topRight, bottomRight, EaseType::EaseOut);
				break;
			case EaseType::EaseInOut:
				drawBezier(topLeft, centerLeft, EaseType::EaseIn);
				drawBezier(centerLeft, bottomLeft, EaseType::EaseOut);
				drawBezier(topRight, centerRight, EaseType::EaseIn);
				drawBezier(centerRight, bottomRight, EaseType::EaseOut);
				break;
			case EaseType::EaseOutIn:
				drawBezier(topLeft, centerLeft, EaseType::EaseOut);
				drawBezier(centerLeft, bottomLeft, EaseType::EaseIn);
				drawBezier(topRight, centerRight, EaseType::EaseOut);
				drawBezier(centerRight, bottomRight, EaseType::EaseIn);
				break;
			default:
			case EaseType::Linear:
				drawBezier(topLeft, bottomLeft, EaseType::Linear);
				drawBezier(topRight, bottomRight, EaseType::Linear);
				break;
			}
		}
	}

	void ScoreEditorTimeline::drawInputNote(Renderer* renderer)
	{
		if (insertingHold)
		{
			drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, false, false,
			              renderer, noteTint);
			drawNote(inputNotes.holdStart, renderer, noteTint);
			drawNote(inputNotes.holdEnd, renderer, noteTint);
		}
		else
		{
			drawNote(currentMode == TimelineMode::InsertLong ? inputNotes.holdStart
			                                                 : inputNotes.tap,
			         renderer, hoverTint);
		}
	}

	void ScoreEditorTimeline::drawHoldNote(const std::unordered_map<id_t, Note>& notes,
	                                       const HoldNote& note, Renderer* renderer,
	                                       const Color& tint_, const int selectedLayer,
	                                       const int offsetTicks, const int offsetLane)
	{
		const Note& start = notes.at(note.start.ID);
		const Note& end = notes.at(note.end);
		const int length = abs(end.tick - start.tick);
		auto tint = tint_;
		if (note.steps.size())
		{
			static constexpr auto isSkipStep = [](const HoldStep& step)
			{ return step.type == HoldStepType::Skip; };
			int s1 = -1;
			int s2 = -1;

			if (length > 0)
			{
				for (int i = 0; i < note.steps.size(); ++i)
				{
					if (isSkipStep(note.steps[i]))
						continue;

					s2 = i;
					const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
					const Note& n2 = s2 == -1 ? end : notes.at(note.steps[s2].ID);
					const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
					const float p1 = (n1.tick - start.tick) / (float)length;
					const float p2 = (n2.tick - start.tick) / (float)length;
					float a1, a2;
					if (!note.isGuide() || note.fadeType == FadeType::None)
					{
						a1 = 1;
						a2 = 1;
					}
					else if (note.fadeType == FadeType::In)
					{
						a1 = p1;
						a2 = p2;
					}
					else
					{
						a1 = 1 - p1;
						a2 = 1 - p2;
					}
					drawHoldCurve(n1, n2, ease, note.isGuide(), note.dummy, renderer, tint,
					              offsetTicks, offsetLane, a1, a2, note.guideColor, selectedLayer);

					s1 = s2;
				}

				const float p1 =
				    s1 == -1 ? 0 : (notes.at(note.steps[s1].ID).tick - start.tick) / (float)length;
				const float p2 = 1;
				float a1, a2;
				if (!note.isGuide() || note.fadeType == FadeType::None)
				{
					a1 = 1;
					a2 = 1;
				}
				else if (note.fadeType == FadeType::In)
				{
					a1 = p1;
					a2 = p2;
				}
				else
				{
					a1 = 1 - p1;
					a2 = 1 - p2;
				}

				const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
				const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
				drawHoldCurve(n1, end, ease, note.isGuide(), note.dummy, renderer, tint,
				              offsetTicks, offsetLane, a1, a2, note.guideColor, selectedLayer);
			}

			s1 = -1;
			s2 = 1;

			if (noteTextures.notes == -1)
				return;

			const Texture& tex = ResourceManager::textures[noteTextures.notes];
			const Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };
			for (int i = 0; i < note.steps.size(); ++i)
			{
				const Note& n3 = notes.at(note.steps[i].ID);

				// find first non-skip step
				s2 = std::distance(note.steps.cbegin(),
				                   std::find_if(note.steps.cbegin() + std::max(s2, i),
				                                note.steps.cend(), std::not_fn(isSkipStep)));

				if (isNoteVisible(n3, offsetTicks))
				{
					if (drawHoldStepOutlines)
						drawSteps.emplace_back(StepDrawData{
						    n3.tick + offsetTicks, n3.lane + (float)offsetLane, n3.width,
						    note.steps[i].type == HoldStepType::Skip ? StepDrawType::SkipStep
						                                             : StepDrawType::NormalStep,
						    n3.layer });

					if (note.steps[i].type != HoldStepType::Hidden)
					{
						int sprIndex = getNoteSpriteIndex(n3);
						if (sprIndex > -1 && sprIndex < tex.sprites.size())
						{
							const Sprite& s = tex.sprites[sprIndex];
							Vector2 pos{ laneToPosition(n3.lane + offsetLane + (n3.width / 2.0f)),
								         getNoteYPosFromTick(n3.tick + offsetTicks) };

							if (isSkipStep(note.steps[i]))
							{
								// find the note before and after the skip step
								const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
								const Note& n2 =
								    s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

								// calculate the interpolation ratio based on the distance between
								// n1 and n2
								float ratio =
								    (float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
								const EaseType rEase =
								    s1 == -1 ? note.start.ease : note.steps[s1].ease;

								auto easeFunc = getEaseFunction(rEase);

								// interpolate the step's position
								float x1 = easeFunc(laneToPosition(n1.lane + offsetLane),
								                    laneToPosition(n2.lane + offsetLane), ratio);
								float x2 = easeFunc(laneToPosition(n1.lane + offsetLane + n1.width),
								                    laneToPosition(n2.lane + offsetLane + n2.width),
								                    ratio);
								pos.x = midpoint(x1, x2);
							}

							int z = (selectedLayer == -1
							             ? (int)ZIndex::zCount
							             : (n3.layer == selectedLayer ? (int)ZIndex::zCount : 0)) +
							        3;

							renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex,
							                     s.getX(), s.getX() + s.getWidth(), s.getY(),
							                     s.getY() + s.getHeight(),
							                     (selectedLayer == -1 || n3.layer == selectedLayer)
							                         ? tint
							                         : tint * otherLayerTint,
							                     z);
							if (n3.dummy)
								drawDummyCrossMark(n3, renderer, tint, offsetTicks, offsetLane);
						}
					}
				}

				// update first interpolation point
				if (!isSkipStep(note.steps[i]))
					s1 = i;
			}
		}
		else
		{
			float a1, a2;
			if (!note.isGuide() || note.fadeType == FadeType::None)
			{
				a1 = 1;
				a2 = 1;
			}
			else if (note.fadeType == FadeType::In)
			{
				a1 = 0;
				a2 = 1;
			}
			else
			{
				a1 = 1;
				a2 = 0;
			}
			drawHoldCurve(start, end, note.start.ease, note.isGuide(), note.dummy, renderer, tint,
			              offsetTicks, offsetLane, a1, a2, note.guideColor, selectedLayer);
		}

		auto inactiveTint = tint * otherLayerTint;

		if (isNoteVisible(start, offsetTicks))
		{
			if (note.startType == HoldNoteType::Normal)
			{
				drawNote(
				    start, renderer,
				    (selectedLayer == -1 || start.layer == selectedLayer) ? tint : inactiveTint,
				    offsetTicks, offsetLane, selectedLayer == -1 || start.layer == selectedLayer);
			}
			else if (drawHoldStepOutlines)
			{
				StepDrawType drawType;
				if (note.startType == HoldNoteType::Guide)
				{
					drawType =
					    static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) +
					                              static_cast<int>(note.guideColor));
				}
				else
				{
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
					                          : StepDrawType::InvisibleHold;
				}
				drawSteps.push_back({ start.tick + offsetTicks, start.lane + (float)offsetLane,
				                      start.width, drawType, start.layer });
			}
		}

		if (isNoteVisible(end, offsetTicks))
		{
			if (note.endType == HoldNoteType::Normal)
			{
				drawNote(end, renderer,
				         (selectedLayer == -1 || end.layer == selectedLayer) ? tint : inactiveTint,
				         offsetTicks, offsetLane,
				         selectedLayer == -1 || start.layer == selectedLayer);
			}
			else if (drawHoldStepOutlines)
			{
				StepDrawType drawType;
				if (note.startType == HoldNoteType::Guide)
				{
					drawType =
					    static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) +
					                              static_cast<int>(note.guideColor));
				}
				else
				{
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
					                          : StepDrawType::InvisibleHold;
				}
				drawSteps.push_back({ end.tick + offsetTicks, end.lane + (float)offsetLane,
				                      end.width, drawType, end.layer });
			}
		}
	}

	void ScoreEditorTimeline::drawHoldMid(Note& note, HoldStepType type, Renderer* renderer,
	                                      const Color& tint, const bool selectedLayer)
	{
		if (type == HoldStepType::Hidden || noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		// Center diamond
		Vector2 pos{ laneToPosition(note.lane + (note.width / 2.0f)),
			         getNoteYPosFromTick(note.tick) };
		Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };

		int z = (selectedLayer ? (int)ZIndex::zCount : 0) + 4;

		renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(),
		                     s.getX() + s.getWidth(), s.getY(), s.getY() + s.getHeight(), tint, z);
	}

	void ScoreEditorTimeline::drawOutline(const StepDrawData& data, int selectedLayer)
	{
		float x = position.x;
		float y = position.y - tickToPosition(data.tick) + visualOffset;

		ImVec2 p1{ x + laneToPosition(data.lane), y - (notesHeight * 0.15f) };
		ImVec2 p2{ x + laneToPosition(data.lane + data.width), y + (notesHeight * 0.15f) };
		auto fill = data.getFillColor();
		auto outline = data.getOutlineColor();

		if (selectedLayer != -1 && data.layer != -1 && selectedLayer != data.layer)
		{
			int fillAlpha = (fill & 0xFF000000) >> 24;
			fill = (fill & 0x00FFFFFF) | (fillAlpha / 2) << 24;
			int outlineAlpha = (outline & 0xFF000000) >> 24;
			outline = (outline & 0x00FFFFFF) | (outlineAlpha / 2) << 24;
		}

		ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, fill, 1.0f, ImDrawFlags_RoundCornersAll);
		ImGui::GetWindowDrawList()->AddRect(p1, p2, outline, 1, ImDrawFlags_RoundCornersAll, 2);
	}

	void ScoreEditorTimeline::drawFlickArrow(const Note& note, Renderer* renderer,
	                                         const Color& tint, const int offsetTick,
	                                         const int offsetLane, const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;

		const Sprite& arrowS = tex.sprites[sprIndex];

		Vector2 pos{ 0, getNoteYPosFromTick(note.tick + offsetTick) };
		const float x1 = laneToPosition(note.lane + offsetLane);
		const float x2 = pos.x + laneToPosition(note.lane + note.width + offsetLane);
		pos.x = midpoint(x1, x2);
		pos.y += notesHeight * 0.7f; // Move the arrow up a bit

		// Notes wider than 6 lanes also use flick arrow size 6
		int sizeIndex = std::min(std::max(note.width, 1.f) - 1, 5.f);
		Vector2 size{ laneWidth * flickArrowWidths[sizeIndex],
			          notesHeight * flickArrowHeights[sizeIndex] };

		float sx1 = arrowS.getX();
		float sx2 = arrowS.getX() + arrowS.getWidth();
		if (note.flick == FlickType::Right || note.flick == FlickType::DownRight)
		{
			// Flip arrow to point to the right
			std::swap(sx1, sx2);
		}
		float sy1 = arrowS.getY();
		float sy2 = arrowS.getY() + arrowS.getHeight();
		if (note.flick >= FlickType::Down)
		{
			// Flip arrow to point to down
			std::swap(sy1, sy2);
		}

		renderer->drawSprite(pos, 0.0f, size, AnchorType::MiddleCenter, tex, sx1, sx2, sy1, sy2,
		                     tint, 2);
	}
	void ScoreEditorTimeline::drawDummyCrossMark(const Note& note, Renderer* renderer,
	                                             const Color& tint, const int offsetTick,
	                                             const int offsetLane, const bool selectedLayer)
	{
		if (noteTextures.dummyNotes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.dummyNotes];
		const int sprIndex = getDummySpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;

		const Sprite& crossSprite = tex.sprites[sprIndex];

		Vector2 pos{ 0, 0 };
		const float x1 = laneToPosition(note.lane + offsetLane);
		const float x2 = laneToPosition(note.lane + note.width + offsetLane);
		pos.x = midpoint(x1, x2);
		pos.y = getNoteYPosFromTick(note.tick + offsetTick);

		float width = std::max(std::min(laneWidth * note.width * 3 / 4, laneWidth * 1.25f),
		                       notesHeight * 0.5f);
		Vector2 size{ width, width };

		const int z = (selectedLayer ? (int)ZIndex::zCount : 0) + (int)ZIndex::Note + 1;

		float sx1 = crossSprite.getX();
		float sx2 = crossSprite.getX() + crossSprite.getWidth();
		float sy1 = crossSprite.getY();
		float sy2 = crossSprite.getY() + crossSprite.getHeight();

		renderer->drawSprite(pos, 0.0f, size, AnchorType::MiddleCenter, tex, sx1, sx2, sy1, sy2,
		                     tint, z);
	}
	void ScoreEditorTimeline::drawNote(const Note& note, Renderer* renderer, const Color& tint,
	                                   const int offsetTick, const int offsetLane,
	                                   const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;

		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane),
			         getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen =
		    std::max(0.0f, (laneWidth * note.width) - (sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		const int z = (selectedLayer ? (int)ZIndex::zCount : 0) + (int)ZIndex::Note;

		// left slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left, left + noteSliceWidth, s.getY(),
		                     s.getY() + s.getHeight(), tint, z);
		pos.x += sliceSz.x;

		// Middle
		renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
		                     left + noteSliceWidth + 1, s.getY(), s.getY() + s.getHeight(), tint,
		                     z);

		pos.x += midLen;

		// right slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth, right,
		                     s.getY(), s.getY() + s.getHeight(), tint, z);

		if (note.friction)
		{
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (isArrayIndexInBounds(frictionSprIndex, tex.sprites))
			{
				// Friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
				                 laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
				    pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
				    frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
				    frictionSpr.getY() + frictionSpr.getHeight(), tint, z + 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
		if (note.dummy)
			drawDummyCrossMark(note, renderer, tint, offsetTick, offsetLane);
	}

	void ScoreEditorTimeline::drawCcNote(const Note& note, Renderer* renderer, const Color& tint,
	                                     const int offsetTick, const int offsetLane,
	                                     const bool selectedLayer)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.ccNotes];
		const int sprIndex = getCcNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane),
			         getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen =
		    std::max(0.0f, (laneWidth * note.width) - (sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		const int z = (selectedLayer ? (int)ZIndex::zCount : 0) + 1;

		// left slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left, left + noteSliceWidth, s.getY(),
		                     s.getY() + s.getHeight(), tint, z);
		pos.x += sliceSz.x;

		// middle
		renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
		                     right - noteSliceWidth, s.getY(), s.getY() + s.getHeight(), tint, z);
		pos.x += midLen;

		// right slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth, right,
		                     s.getY(), s.getY() + s.getHeight(), tint, z);

		if (note.friction)
		{
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (frictionSprIndex >= 0 && frictionSprIndex < tex.sprites.size())
			{
				// friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
				                 laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
				    pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
				    frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
				    frictionSpr.getY() + frictionSpr.getHeight(), tint, z + 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
		if (note.dummy)
			drawDummyCrossMark(note, renderer, tint, offsetTick, offsetLane);
	}

	bool ScoreEditorTimeline::bpmControl(const ScoreContext& context, const Tempo& tempo)
	{
		return bpmControl(context, tempo.bpm, tempo.tick, !playing);
	}

	bool ScoreEditorTimeline::bpmControl(const ScoreContext& context, float bpm, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		// タイムラインの左端（StartX）から左へ110pxの位置に配置
		Vector2 pos{ getTimelineStartX(context) - (110.0f * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(context), pos, tempoColor,
		                    IO::formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditorTimeline::timeSignatureControl(const ScoreContext& context, int numerator, int denominator, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		// タイムラインの左端（StartX）から左へ45pxの位置に配置
		Vector2 pos{ getTimelineStartX(context) - (45.0f * dpiScale),
			         position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(context), pos, timeColor,
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
		return eventControl(getTimelineStartX(context), pos, skillBadgeColor(effect),
		                    IO::formatString("%s Lv.%d", skillBadgeIcon(effect), level).c_str(),
		                    enabled);
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
		eventControl(getTimelineEndX(context), pos, feverColor, txt.c_str(), enabled,
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

		return eventControl(
		    hiSpeedStartX, pos,
		    selected ? ImGui::ColorConvertFloat4ToU32(generateHighlightColor(
		                   generateHighlightColor(ImGui::ColorConvertU32ToFloat4(color))))
		             : color,
		    txt.c_str(), enabled);
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
		return eventControl(getTimelineStartX(context), pos, waypointColor, name.c_str(), true);
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

				switch (eventEdit.editSkillEffect)
				{
				case SkillEffect::Score:
					ImGui::TextWrapped("%s", getString("skill_effect_score_desc"));
					break;
				case SkillEffect::Heal:
					ImGui::TextWrapped("%s", getString("skill_effect_heal_desc"));
					break;
				case SkillEffect::Perfect:
					ImGui::TextWrapped("%s", getString("skill_effect_perfect_desc"));
					break;
				default:
					break;
				}
				ImGui::TextDisabled("%s", getString("skill_level_desc"));

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

	bool ScoreEditorTimeline::isMouseInHoldPath(const Note& n1, const Note& n2, EaseType ease, float x, float y)
	{
		float xStart1 = laneToPosition(n1.lane);
		float xStart2 = laneToPosition(n1.lane + n1.width);
		float xEnd1 = laneToPosition(n2.lane);
		float xEnd2 = laneToPosition(n2.lane + n2.width);
		float y1 = getNoteYPosFromTick(n1.tick);
		float y2 = getNoteYPosFromTick(n2.tick);

		if (!isWithinRange(y, y1, y2))
			return false;

		auto easeFunc = getEaseFunction(ease);
		float percent = (y - y1) / (y2 - y1);
		float x1 = easeFunc(xStart1, xEnd1, percent);
		float x2 = easeFunc(xStart2, xEnd2, percent);

		return isWithinRange(x, std::min(x1, x2), std::max(x1, x2));
	}

	void ScoreEditorTimeline::previousTick(ScoreContext& context)
	{
		if (playing || context.currentTick == 0)
			return;

		context.currentTick = std::max(
		    roundTickDown(context.currentTick, division) - (TICKS_PER_BEAT / (division / 4)), 0);
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Down);
	}

	void ScoreEditorTimeline::nextTick(ScoreContext& context)
	{
		if (playing)
			return;

		context.currentTick =
		    roundTickDown(context.currentTick, division) + (TICKS_PER_BEAT / (division / 4));
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Up);
	}

	int ScoreEditorTimeline::roundTickDown(int tick, int division) const
	{
		return std::max(tick - (tick % (TICKS_PER_BEAT / (division / 4))), 0);
	}

	void ScoreEditorTimeline::insertNote(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note newNote = inputNotes.tap;
		newNote.ID = Note::getNextID();
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert note", prev, context.score);
	}

	void ScoreEditorTimeline::insertHold(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note holdStart = inputNotes.holdStart;
		holdStart.ID = Note::getNextID();
		holdStart.layer = context.selectedLayer;

		Note holdEnd = inputNotes.holdEnd;
		holdEnd.ID = Note::getNextID();
		holdEnd.parentID = holdStart.ID;
		holdEnd.layer = context.selectedLayer;

		HoldEndType startType = edit.holdStartType;
		HoldEndType endType = edit.holdEndType;
		if (holdStart.tick == holdEnd.tick)
		{
			holdEnd.tick += TICKS_PER_BEAT / (static_cast<float>(division) / 4);
		}
		else if (holdStart.tick > holdEnd.tick)
		{
			std::swap(holdStart.tick, holdEnd.tick);
			std::swap(holdStart.width, holdEnd.width);
			std::swap(holdStart.lane, holdEnd.lane);
			std::swap(startType, endType);
		}

		HoldNoteType holdStartType;
		if (currentMode == TimelineMode::InsertGuide)
		{
			holdStartType = HoldNoteType::Guide;
		}
		else if (startType == HoldEndType::Hidden)
		{
			holdStartType = HoldNoteType::Hidden;
		}
		else
		{
			holdStartType = HoldNoteType::Normal;
		}

		HoldNoteType holdEndType;
		if (currentMode == TimelineMode::InsertGuide)
		{
			holdEndType = HoldNoteType::Guide;
		}
		else if (endType == HoldEndType::Hidden)
		{
			holdEndType = HoldNoteType::Hidden;
		}
		else
		{
			holdEndType = HoldNoteType::Normal;
		}

		context.score.notes[holdStart.ID] = holdStart;
		context.score.notes[holdEnd.ID] = holdEnd;
		context.score.holdNotes[holdStart.ID] = { {
			                                          holdStart.ID,
			                                          HoldStepType::Normal,
			                                          edit.easeType,
			                                      },
			                                      {},
			                                      holdEnd.ID,
			                                      holdStartType,
			                                      holdEndType,
			                                      edit.fadeType,
			                                      edit.colorType };
		context.pushHistory("Insert hold", prev, context.score);
	}

	void ScoreEditorTimeline::insertHoldStep(ScoreContext& context, EditArgs& edit, int holdId)
	{
		// make sure the hold data exists
		if (context.score.holdNotes.find(holdId) == context.score.holdNotes.end())
			return;

		// make sure the parent hold note exists
		if (context.score.notes.find(holdId) == context.score.notes.end())
			return;

		Score prev = context.score;

		HoldNote& hold = context.score.holdNotes[holdId];
		Note holdStart = context.score.notes[holdId];

		Note holdStep = inputNotes.holdStep;
		holdStep.ID = Note::getNextID();
		holdStep.critical = holdStart.critical;
		holdStep.parentID = holdStart.ID;
		holdStep.layer = context.selectedLayer;

		context.score.notes[holdStep.ID] = holdStep;

		hold.steps.push_back(
		    { holdStep.ID, hold.isGuide() ? HoldStepType::Hidden : edit.stepType, edit.easeType });

		// sort steps in-case the step is inserted before/after existing steps
		sortHoldSteps(context.score, hold);
		context.pushHistory("Insert hold step", prev, context.score);
	}

	void ScoreEditorTimeline::insertDamage(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note newNote = inputNotes.damage;
		newNote.ID = Note::getNextID();
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert damage", prev, context.score);
	}

	void ScoreEditorTimeline::debug(ScoreContext& context)
	{
		ImGui::Text("Window position: (%.2f, %.2f)\nWindow size: (%.2f, %.2f)", position.x,
		            position.y, size.x, size.y);
		ImGui::Separator();

		ImGui::Text("Mouse position: (%.2f, %.2f)\nMouse in timeline: %s", mousePos.x, mousePos.y,
		            boolToString(mouseInTimeline));
		ImGui::Separator();

		ImGui::Text("Minimum offset: %g\nCurrent offset: %g\nMaximum offset: %g", minOffset, offset,
		            maxOffset);
		ImGui::Separator();

		if (mouseInTimeline)
		{
			ImGui::Text("Hover lane: %d\nHover tick: %d", hoverLane, hoverTick);
		}
		else
		{
			ImGui::TextDisabled("Hover lane: --\nHover tick: --");
		}

		ImGui::Text("Last selected tick : %d", lastSelectedTick);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Hover Note", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Hovering note ID: %llu", hoveringNote);
			ImGui::Text("Holding note ID: %llu", holdingNote);

			auto it = context.score.notes.find(hoveringNote);
			if (it != context.score.notes.end())
			{
				const Note& note = it->second;
				ImGui::Text("ID: %llu\nType: %hhu\nTick: %d\nLane: %f\nWidth: %f\nCritical: "
				            "%s\nFriction: %s\nFlick: %s",
				            note.ID, static_cast<uint8_t>(note.getType()), note.tick, note.lane,
				            note.width, boolToString(note.critical), boolToString(note.friction),
				            flickTypes[static_cast<int>(note.flick)]);
			}
			else
			{
				// Prevent window scrolling from jumping around
				ImGui::TextDisabled("ID: -\nType: -\nTick: -\nLane: -\nWidth: -\nCritical: "
				                    "-\nFriction: -\nFlick: -");
			}
		}
	}

		ScoreEditorTimeline::ScoreEditorTimeline()
	{
		framebuffer = std::make_unique<Framebuffer>(1920, 1080);
		playbackSpeed = 1.0f;

		background.load(config.backgroundImage.empty()
		                    ? (Application::getAppDir() + "res\\textures\\default.png")
		                    : config.backgroundImage);
		background.setBrightness(0.67);

		timelineInstance = this;
	}

	void ScoreEditorTimeline::setPlaybackSpeed(ScoreContext& context, float speed)
	{
		playbackSpeed = std::clamp(speed, minPlaybackSpeed, maxPlaybackSpeed);
		context.audio.setPlaybackSpeed(playbackSpeed, time);
	}

	void ScoreEditorTimeline::setPlaying(ScoreContext& context, bool state)
	{
		if (playing == state)
			return;

		playing = state;
		if (playing)
		{
			playStartTime = time;
			context.audio.seekMusic(time);
			context.audio.playMusic(time);
			context.audio.setLastPlaybackTime(time);
			context.audio.syncAudioEngineTimer();
		}
		else
		{
			if (config.returnToLastSelectedTickOnPause)
			{
				context.currentTick = lastSelectedTick;
				offset =
				    std::max(minOffset, tickToPosition(context.currentTick) +
				                            (size.y * (1.0f - config.cursorPositionThreshold)));
			}

			context.audio.stopSoundEffects(false);
			context.audio.stopMusic();
		}
	}

	void ScoreEditorTimeline::stop(ScoreContext& context)
	{
		playing = false;
		time = lastSelectedTick = context.currentTick = 0;
		offset = std::max(minOffset, tickToPosition(context.currentTick) +
		                                 (size.y * (1.0f - config.cursorPositionThreshold)));

		context.audio.stopSoundEffects(false);
		context.audio.stopMusic();
	}

	void ScoreEditorTimeline::updateNoteSE(ScoreContext& context)
	{
		if (!playing)
			return;

		auto singleNoteSEFunc = [&context, this](const Note& note, float notePlayTime)
		{
			bool playSE = true;
			if (note.getType() == NoteType::Hold)
			{
				playSE = context.score.holdNotes.at(note.ID).startType == HoldNoteType::Normal;
			}
			else if (note.getType() == NoteType::HoldEnd)
            {
                if (context.score.holdNotes.find(note.parentID) != context.score.holdNotes.end()) 
                {
                    playSE = context.score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
                }
                else 
                {
                    playSE = false;
                }
            }
			if (note.isHold())
			{
				const id_t holdId = note.getType() == NoteType::Hold ? note.ID : note.parentID;
				const auto holdIt = context.score.holdNotes.find(holdId);
				if (holdIt != context.score.holdNotes.end() && holdIt->second.dummy)
					playSE = false;
			}
			if (note.dummy)
			{
				playSE = false;
			}

			if (playSE)
			{
				std::string_view se = getNoteSE(note, context.score);
				std::string key = std::to_string(note.tick) + "-" + se.data();
				if (!se.empty() && (playingNoteSounds.find(key) == playingNoteSounds.end()))
				{
					context.audio.playSoundEffect(se.data(), notePlayTime, -1, time);
					playingNoteSounds.insert(key);
				}
			}
		};

		auto holdNoteSEFunc = [&context, this](const Note& note, float startTime)
		{
			HoldNote& hold = context.score.holdNotes.at(note.ID);
			if (hold.dummy)
				return;
			int endTick = context.score.notes.at(hold.end).tick;
			if (endTick <= note.tick)
				return;

			const std::string_view se = note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT;
			const std::vector<int> boundaries =
			    getHiSpeedBoundaryTicks(context.score, note.layer, note.tick, endTick);
			for (size_t i = 0; i + 1 < boundaries.size(); ++i)
			{
				const int segmentStartTick = boundaries[i];
				const int segmentEndTick = boundaries[i + 1];
				if (isHideNotesActive(context.score, note.layer, segmentStartTick))
					continue;

				const float segmentStartTime =
				    accumulateDuration(segmentStartTick, TICKS_PER_BEAT, context.score.tempoChanges);
				const float segmentEndTime =
				    accumulateDuration(segmentEndTick, TICKS_PER_BEAT, context.score.tempoChanges);
				const float adjustedStartTime =
				    std::max(startTime, segmentStartTime - playStartTime - audioOffsetCorrection);
				const float adjustedEndTime = segmentEndTime - playStartTime + audioOffsetCorrection;
				if (adjustedEndTime <= adjustedStartTime)
					continue;

				const float engineTime = context.audio.getAudioEngineAbsoluteTime();
				const float scheduledEndTime =
				    adjustedEndTime + playbackSpeed * (adjustedStartTime - engineTime);
				context.audio.playSoundEffect(se.data(), adjustedStartTime, scheduledEndTime, time);
			}
		};

		playingNoteSounds.clear();
		for (const auto& [id, note] : context.score.notes)
		{
			float noteTime =
			    accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float notePlayTime = noteTime - playStartTime;
			float offsetNoteTime = noteTime - (audioLookAhead * playbackSpeed);

			if (offsetNoteTime >= timeLastFrame && offsetNoteTime < time)
			{
				singleNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
					holdNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
			}
			else if (time == playStartTime)
			{
				// Playback just started
				if (noteTime >= time && offsetNoteTime < time)
					singleNoteSEFunc(note, notePlayTime);

				// Playback started mid-hold
				if (note.getType() == NoteType::Hold &&
				    !context.score.holdNotes.at(note.ID).isGuide())
				{
					int endTick =
					    context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
					float endTime =
					    accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);
					if ((noteTime - time) <= audioLookAhead && endTime > time)
						holdNoteSEFunc(note, std::max(0.0f, notePlayTime));
				}
			}
		}
	}

	ImRect ScoreEditorTimeline::getAudioClipRect(ScoreContext& context, const AudioClip& clip) const
	{
		const float dpiScale = ImGui::GetMainViewport()->DpiScale;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const int startTick = accumulateTicks(clip.timelineStartMs / 1000.0f, TICKS_PER_BEAT,
		                                      context.score.tempoChanges);
		const int endTick = accumulateTicks((clip.timelineStartMs + durationMs) / 1000.0f,
		                                    TICKS_PER_BEAT, context.score.tempoChanges);
		const float y1 = position.y - tickToPosition(startTick) + visualOffset;
		const float y2 = position.y - tickToPosition(endTick) + visualOffset;
		const float minX = getTimelineStartX(context) + (8.0f * dpiScale);
		const float maxX = getTimelineEndX(context) - (8.0f * dpiScale);
		const float availableWidth = std::max(1.0f, maxX - minX);
		const float clipWidth = std::min(260.0f * dpiScale, availableWidth);
		const float centerX = midpoint(getTimelineStartX(context), getTimelineEndX(context));
		const float x1 = std::clamp(centerX - (clipWidth * 0.5f), minX, maxX - clipWidth);
		const float x2 = x1 + clipWidth;
		return ImRect(ImVec2(x1, std::min(y1, y2)), ImVec2(x2, std::max(y1, y2)));
	}

	float ScoreEditorTimeline::getTimelineMsAtScreenY(const ScoreContext& context,
	                                                  float screenY) const
	{
		const float tickPosition = position.y + visualOffset - screenY;
		const float rawTick =
		    std::max(0.0f, tickPosition / std::max(0.0001f, unitHeight * zoom));
		const int baseTick = static_cast<int>(std::floor(rawTick));
		const float fraction = rawTick - baseTick;
		const float start =
		    accumulateDuration(baseTick, TICKS_PER_BEAT, context.score.tempoChanges);
		const float end =
		    accumulateDuration(baseTick + 1, TICKS_PER_BEAT, context.score.tempoChanges);
		return (start + ((end - start) * fraction)) * 1000.0f;
	}

	float ScoreEditorTimeline::getScreenYFromTimelineMs(const ScoreContext& context,
	                                                    float timelineMs) const
	{
		const int tick = accumulateTicks(timelineMs / 1000.0f, TICKS_PER_BEAT,
		                                 context.score.tempoChanges);
		return position.y - tickToPosition(tick) + visualOffset;
	}

	float ScoreEditorTimeline::snapTimelineMs(const ScoreContext& context, float timelineMs,
	                                          bool roundDown) const
	{
		const int tick = accumulateTicks(timelineMs / 1000.0f, TICKS_PER_BEAT,
		                                 context.score.tempoChanges);
		const int snappedTick = roundDown ? roundTickDown(tick, division) : snapTick(tick, division);
		return static_cast<float>(accumulateDuration(snappedTick, TICKS_PER_BEAT,
		                                             context.score.tempoChanges) *
		                          1000.0);
	}

	float ScoreEditorTimeline::getMouseTimelineMs(const ScoreContext& context, bool snap) const
	{
		const float timelineMs = getTimelineMsAtScreenY(context, ImGui::GetIO().MousePos.y);
		if (!snap)
			return timelineMs;
		return snapTimelineMs(context, timelineMs, snapMode != SnapMode::Relative);
	}

	void ScoreEditorTimeline::drawAudioClipWaveform(ScoreContext& context, const AudioClip& clip,
	                                                const ImRect& rect, ImU32 leftColor,
	                                                ImU32 rightColor)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList || context.sourceWaveformL.isEmpty())
			return;

		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const float clipPadding = 8.0f * ImGui::GetMainViewport()->DpiScale;
		const float centerX = midpoint(rect.Min.x, rect.Max.x);
		const float maxBarValue = std::max(1.0f, (rect.GetWidth() * 0.5f) - clipPadding);
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);

		drawList->PushClipRect(rect.Min, rect.Max, true);
		const float screenYStart = std::max(rect.Min.y, position.y);
		const float screenYEnd = std::min(rect.Max.y, position.y + size.y);
		const int timelineYStart =
		    static_cast<int>(std::floor(position.y + visualOffset - screenYEnd));
		const int timelineYEnd =
		    static_cast<int>(std::ceil(position.y + visualOffset - screenYStart));
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		for (size_t channelIndex = 0; channelIndex < 2; ++channelIndex)
		{
			const bool rightChannel = channelIndex == 1;
			const Audio::WaveformMipChain& waveform =
			    rightChannel ? context.sourceWaveformR : context.sourceWaveformL;
			if (waveform.isEmpty())
				continue;

			const ImU32 color = rightChannel ? rightColor : leftColor;
			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);
			for (int timelineY = timelineYStart; timelineY <= timelineYEnd; ++timelineY)
			{
				const float screenY = std::floor(position.y + visualOffset - timelineY);
				if (screenY < screenYStart || screenY > screenYEnd)
					continue;

				const int tick = positionToTick(timelineY);
				const float timelineMs =
				    static_cast<float>(accumulateDuration(tick, TICKS_PER_BEAT,
				                                          context.score.tempoChanges) *
				                       1000.0);
				const float sourceMs = clip.sourceStartMs + (timelineMs - clip.timelineStartMs);
				if (sourceMs < clip.sourceStartMs || sourceMs > sourceEndMs)
					continue;

				const float clipTimeMs = sourceMs - clip.sourceStartMs;
				float envelope = clip.gain;
				if (clip.fadeInMs > 0.0f)
					envelope *= std::clamp(clipTimeMs / clip.fadeInMs, 0.0f, 1.0f);
				if (clip.fadeOutMs > 0.0f)
					envelope *= std::clamp((durationMs - clipTimeMs) / clip.fadeOutMs, 0.0f, 1.0f);

				const double sourceSeconds = sourceMs / 1000.0;
				const float amplitude = std::max(
				    waveform.getAmplitudeAt(mip, sourceSeconds, secondsPerPixel), 0.0f);
				const float barValue = amplitude * envelope * maxBarValue;
				const float halfBar = std::max(0.75f, barValue);
				const ImVec2 p1(centerX, screenY);
				const ImVec2 p2(centerX + (halfBar * (rightChannel ? 1.0f : -1.0f)),
				                screenY + 0.75f);
				drawList->AddRectFilled(p1, p2, color);
			}
		}
		drawList->PopClipRect();
	}

	bool ScoreEditorTimeline::hasAudioRangeSelection(id_t clipId) const
	{
		if (audioRangeClip != clipId)
			return false;
		return std::abs(audioRangeEndMs - audioRangeStartMs) >= 1.0f;
	}

	bool ScoreEditorTimeline::splitAudioClipAt(ScoreContext& context, id_t clipId,
	                                           float timelineMs)
	{
		for (AudioClip& clip : context.score.audioTrack.clips)
		{
			if (clip.ID != clipId)
				continue;

			const float sourceEndMs =
			    clip.sourceEndMs >= 0.0f
			        ? clip.sourceEndMs
			        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
			const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
			const float clipStartMs = clip.timelineStartMs;
			const float clipEndMs = clip.timelineStartMs + durationMs;
			if (timelineMs <= clipStartMs + 1.0f || timelineMs >= clipEndMs - 1.0f)
				return false;

			const Score prev = context.score;
			const float splitSourceMs = clip.sourceStartMs + (timelineMs - clipStartMs);

			AudioClip rightClip = clip;
			rightClip.ID = getNextAudioClipID();
			rightClip.sourceStartMs = splitSourceMs;
			rightClip.timelineStartMs = timelineMs;
			rightClip.fadeInMs = 0.0f;

			clip.sourceEndMs = splitSourceMs;
			clip.fadeOutMs = 0.0f;

			context.score.audioTrack.clips.push_back(rightClip);
			context.score.audioTrack.explicitEditorTrack = true;
			context.selectedAudioClip = rightClip.ID;
			audioRangeClip = static_cast<id_t>(-1);
			AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
			context.pushHistory("Split Audio Clip", prev, context.score);
			Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
			if (!refreshResult.isOk())
				IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
				               IO::MessageBoxIcon::Warning);
			return true;
		}

		return false;
	}

	bool ScoreEditorTimeline::splitAudioClipRange(ScoreContext& context, id_t clipId, float startMs,
	                                              float endMs)
	{
		if (endMs < startMs)
			std::swap(startMs, endMs);
		if (endMs - startMs < 1.0f)
			return false;

		auto& clips = context.score.audioTrack.clips;
		auto it = std::find_if(clips.begin(), clips.end(),
		                       [clipId](const AudioClip& clip) { return clip.ID == clipId; });
		if (it == clips.end())
			return false;

		const AudioClip clip = *it;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const float clipStartMs = clip.timelineStartMs;
		const float clipEndMs = clip.timelineStartMs + durationMs;
		const float splitStartMs = std::clamp(startMs, clipStartMs, clipEndMs);
		const float splitEndMs = std::clamp(endMs, clipStartMs, clipEndMs);
		const bool hasStartSplit = splitStartMs > clipStartMs + 1.0f;
		const bool hasEndSplit = splitEndMs < clipEndMs - 1.0f;
		if (!hasStartSplit && !hasEndSplit)
			return false;

		const Score prev = context.score;
		std::vector<AudioClip> replacementClips;

		auto makeSegment = [&](float segmentStartMs, float segmentEndMs, bool keepOriginalId)
		{
			AudioClip segment = clip;
			segment.ID = keepOriginalId ? clip.ID : getNextAudioClipID();
			segment.timelineStartMs = segmentStartMs;
			segment.sourceStartMs = clip.sourceStartMs + (segmentStartMs - clipStartMs);
			segment.sourceEndMs = clip.sourceStartMs + (segmentEndMs - clipStartMs);
			if (segmentStartMs > clipStartMs + 1.0f)
				segment.fadeInMs = 0.0f;
			if (segmentEndMs < clipEndMs - 1.0f)
				segment.fadeOutMs = 0.0f;
			replacementClips.push_back(segment);
		};

		const float middleStartMs = hasStartSplit ? splitStartMs : clipStartMs;
		const float middleEndMs = hasEndSplit ? splitEndMs : clipEndMs;
		if (hasStartSplit)
			makeSegment(clipStartMs, splitStartMs, true);
		makeSegment(middleStartMs, middleEndMs, !hasStartSplit);
		if (hasEndSplit)
			makeSegment(splitEndMs, clipEndMs, false);

		const size_t index = static_cast<size_t>(std::distance(clips.begin(), it));
		clips.erase(clips.begin() + index);
		clips.insert(clips.begin() + index, replacementClips.begin(), replacementClips.end());
		context.score.audioTrack.explicitEditorTrack = true;
		context.selectedAudioClip = replacementClips.size() > 1
		                                ? replacementClips[hasStartSplit ? 1 : 0].ID
		                                : replacementClips.front().ID;
		audioRangeClip = context.selectedAudioClip;
		audioRangeStartMs = middleStartMs;
		audioRangeEndMs = middleEndMs;
		AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
		context.pushHistory("Split Audio Range", prev, context.score);
		Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
		if (!refreshResult.isOk())
			IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning);
		return true;
	}

	bool ScoreEditorTimeline::cutAudioClipRange(ScoreContext& context, id_t clipId, float startMs,
	                                            float endMs)
	{
		if (endMs < startMs)
			std::swap(startMs, endMs);
		if (endMs - startMs < 1.0f)
			return false;

		auto& clips = context.score.audioTrack.clips;
		auto it = std::find_if(clips.begin(), clips.end(),
		                       [clipId](const AudioClip& clip) { return clip.ID == clipId; });
		if (it == clips.end())
			return false;

		const AudioClip clip = *it;
		const float sourceEndMs =
		    clip.sourceEndMs >= 0.0f
		        ? clip.sourceEndMs
		        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
		const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
		const float clipStartMs = clip.timelineStartMs;
		const float clipEndMs = clip.timelineStartMs + durationMs;
		const float cutStartMs = std::clamp(startMs, clipStartMs, clipEndMs);
		const float cutEndMs = std::clamp(endMs, clipStartMs, clipEndMs);
		if (cutEndMs - cutStartMs < 1.0f)
			return false;

		const Score prev = context.score;
		std::vector<AudioClip> replacementClips;
		const float leftDurationMs = cutStartMs - clipStartMs;
		const float rightOffsetMs = cutEndMs - clipStartMs;

		if (leftDurationMs > 1.0f)
		{
			AudioClip leftClip = clip;
			leftClip.sourceEndMs = clip.sourceStartMs + leftDurationMs;
			if (cutEndMs < clipEndMs - 1.0f)
				leftClip.fadeOutMs = 0.0f;
			replacementClips.push_back(leftClip);
		}

		if (cutEndMs < clipEndMs - 1.0f)
		{
			AudioClip rightClip = clip;
			rightClip.ID = getNextAudioClipID();
			rightClip.sourceStartMs = clip.sourceStartMs + rightOffsetMs;
			rightClip.sourceEndMs = sourceEndMs;
			rightClip.timelineStartMs = cutEndMs;
			if (cutStartMs > clipStartMs + 1.0f)
				rightClip.fadeInMs = 0.0f;
			replacementClips.push_back(rightClip);
		}

		const size_t index = static_cast<size_t>(std::distance(clips.begin(), it));
		clips.erase(clips.begin() + index);
		clips.insert(clips.begin() + index, replacementClips.begin(), replacementClips.end());
		context.score.audioTrack.explicitEditorTrack = true;
		context.selectedAudioClip =
		    replacementClips.empty() ? static_cast<id_t>(-1) : replacementClips.back().ID;
		audioRangeClip = static_cast<id_t>(-1);
		AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
		context.pushHistory("Cut Audio Range", prev, context.score);
		Result refreshResult = AudioTrackUtils::refreshPlaybackAudio(context);
		if (!refreshResult.isOk())
			IO::messageBox(APP_NAME, refreshResult.getMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning);
		return true;
	}

	void ScoreEditorTimeline::drawAudioTrack(ScoreContext& context)
	{
		const bool audioEditing = context.isAudioEditing();
		if (!audioEditing && !config.drawWaveform)
			return;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		for (const AudioClip& clip : context.score.audioTrack.clips)
		{
			if (!clip.visible)
				continue;

			const ImRect rect = getAudioClipRect(context, clip);
			if (rect.Max.y < position.y || rect.Min.y > position.y + size.y)
				continue;

			const bool selected = audioEditing && context.selectedAudioClip == clip.ID;
			const ImU32 fillColor =
			    audioEditing ? (selected ? IM_COL32(48, 78, 96, 92)
			                             : IM_COL32(48, 78, 96, 48))
			                 : IM_COL32(48, 78, 96, 18);
			const ImU32 borderColor =
			    selected ? IM_COL32(165, 220, 255, 230) : IM_COL32(135, 180, 210, 130);
			drawList->AddRectFilled(rect.Min, rect.Max, fillColor, 4.0f);
			drawAudioClipWaveform(context, clip, rect,
			                      audioEditing ? IM_COL32(185, 218, 238, 150)
			                                   : IM_COL32(185, 218, 238, 58),
			                      audioEditing ? IM_COL32(155, 195, 220, 130)
			                                   : IM_COL32(155, 195, 220, 45));

			if (audioEditing && hasAudioRangeSelection(clip.ID))
			{
				const float rangeStartMs = std::min(audioRangeStartMs, audioRangeEndMs);
				const float rangeEndMs = std::max(audioRangeStartMs, audioRangeEndMs);
				const float y1 = getScreenYFromTimelineMs(context, rangeStartMs);
				const float y2 = getScreenYFromTimelineMs(context, rangeEndMs);
				const ImRect rangeRect(ImVec2(rect.Min.x, std::min(y1, y2)),
				                       ImVec2(rect.Max.x, std::max(y1, y2)));
				const ImRect clippedRange(ImVec2(rangeRect.Min.x, std::max(rangeRect.Min.y, rect.Min.y)),
				                          ImVec2(rangeRect.Max.x, std::min(rangeRect.Max.y, rect.Max.y)));
				if (clippedRange.Max.y > clippedRange.Min.y)
				{
					drawList->AddRectFilled(clippedRange.Min, clippedRange.Max,
					                        IM_COL32(255, 255, 255, 38), 2.0f);
					drawList->AddRect(clippedRange.Min, clippedRange.Max,
					                  IM_COL32(255, 255, 255, 130), 2.0f);
				}
			}

			drawList->AddRect(rect.Min, rect.Max, borderColor, 4.0f, 0,
			                  audioEditing ? 2.0f : 1.0f);

			if (audioEditing)
			{
				const float dpiScale = ImGui::GetMainViewport()->DpiScale;
				const float grabWidth = 14.0f * dpiScale;
				const float knobRadius = 4.0f * dpiScale;
				const float sourceEndMs =
				    clip.sourceEndMs >= 0.0f
				        ? clip.sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float durationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
				const ImU32 handleColor =
				    context.score.audioTrack.locked || clip.locked ? IM_COL32(180, 180, 180, 120)
				                                                   : IM_COL32(225, 240, 255, 180);
				drawList->AddRectFilled(rect.Min, ImVec2(rect.Min.x + grabWidth, rect.Max.y),
				                        selected ? IM_COL32(130, 190, 230, 92)
				                                 : IM_COL32(130, 190, 230, 55),
				                        4.0f, ImDrawFlags_RoundCornersLeft);

				const float fadeInY =
				    std::clamp(getScreenYFromTimelineMs(context, clip.timelineStartMs + clip.fadeInMs),
				               rect.Min.y, rect.Max.y);
				const float fadeOutY = std::clamp(
				    getScreenYFromTimelineMs(context,
				                             clip.timelineStartMs + durationMs - clip.fadeOutMs),
				    rect.Min.y, rect.Max.y);
				const ImVec2 fadeInHandle(rect.Min.x + (14.0f * dpiScale), fadeInY);
				const ImVec2 fadeOutHandle(rect.Max.x - (14.0f * dpiScale), fadeOutY);
				drawList->AddLine(ImVec2(rect.Min.x, fadeInY), ImVec2(rect.Max.x, fadeInY),
				                  IM_COL32(255, 255, 255, clip.fadeInMs > 0.0f ? 110 : 45),
				                  clip.fadeInMs > 0.0f ? 1.5f : 1.0f);
				drawList->AddLine(ImVec2(rect.Min.x, fadeOutY), ImVec2(rect.Max.x, fadeOutY),
				                  IM_COL32(255, 255, 255, clip.fadeOutMs > 0.0f ? 110 : 45),
				                  clip.fadeOutMs > 0.0f ? 1.5f : 1.0f);
				drawList->AddCircleFilled(fadeInHandle, knobRadius, handleColor);
				drawList->AddCircleFilled(fadeOutHandle, knobRadius, handleColor);

				const std::string clipName =
				    clip.sourceFile.empty() ? context.workingData.musicFilename : clip.sourceFile;
				const std::string label = IO::File::getFilename(clipName);
				if (!label.empty() && rect.GetHeight() > ImGui::GetTextLineHeight() + 8.0f)
				{
					drawList->AddText(ImVec2(rect.Min.x + 6.0f * dpiScale,
					                         rect.Min.y + 6.0f * dpiScale),
					                  IM_COL32(230, 240, 248, 170), label.c_str());
				}
			}
		}
	}

	void ScoreEditorTimeline::updateAudioTrackEditing(ScoreContext& context)
	{
		suppressTimelineContextMenu = false;
		if (!context.isAudioEditing() || context.score.audioTrack.locked)
			return;

		const ImGuiIO& io = ImGui::GetIO();
		const bool useSnap = io.KeyShift;
		const float currentTimelineMs = getMouseTimelineMs(context, useSnap);
		const float playheadTimelineMs = static_cast<float>(
		    accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges) *
		    1000.0);

		auto findClip = [&](id_t id) -> AudioClip*
		{
			for (AudioClip& clip : context.score.audioTrack.clips)
			{
				if (clip.ID == id)
					return &clip;
			}
			return nullptr;
		};

		auto warnRefreshFailure = [](const Result& result)
		{
			if (!result.isOk())
				IO::messageBox(APP_NAME, result.getMessage(), IO::MessageBoxButtons::Ok,
				               IO::MessageBoxIcon::Warning);
		};

		if (ImGui::BeginPopup("audio_clip_context"))
		{
			AudioClip* menuClip = findClip(audioContextMenuClip);
			bool canCut = menuClip && audioContextMenuHasRange;
			bool canSplit = false;
			if (menuClip)
			{
				const float sourceEndMs =
				    menuClip->sourceEndMs >= 0.0f
				        ? menuClip->sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float durationMs = std::max(1.0f, sourceEndMs - menuClip->sourceStartMs);
				if (audioContextMenuHasRange)
				{
					const float rangeStartMs =
					    std::min(audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
					const float rangeEndMs =
					    std::max(audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
					canSplit = rangeStartMs > menuClip->timelineStartMs + 1.0f ||
					           rangeEndMs < menuClip->timelineStartMs + durationMs - 1.0f;
				}
				else
				{
					canSplit = playheadTimelineMs > menuClip->timelineStartMs + 1.0f &&
					           playheadTimelineMs < menuClip->timelineStartMs + durationMs - 1.0f;
				}
			}

			if (ImGui::MenuItem(getString("cut"), nullptr, false, canCut))
			{
				cutAudioClipRange(context, audioContextMenuClip,
				                  audioContextMenuRangeStartMs, audioContextMenuRangeEndMs);
				audioContextMenuHasRange = false;
			}
			if (ImGui::MenuItem(getString("audio_clip_split"), nullptr, false, canSplit))
			{
				if (audioContextMenuHasRange)
				{
					splitAudioClipRange(context, audioContextMenuClip,
					                    audioContextMenuRangeStartMs,
					                    audioContextMenuRangeEndMs);
				}
				else
				{
					splitAudioClipAt(context, audioContextMenuClip, playheadTimelineMs);
				}
				audioContextMenuHasRange = false;
			}
			ImGui::EndPopup();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Delete) &&
		    context.selectedAudioClip != static_cast<id_t>(-1))
		{
			if (hasAudioRangeSelection(context.selectedAudioClip))
			{
				cutAudioClipRange(context, context.selectedAudioClip, audioRangeStartMs,
				                  audioRangeEndMs);
				return;
			}

			Score prev = context.score;
			context.score.audioTrack.explicitEditorTrack = true;
			auto& clips = context.score.audioTrack.clips;
			clips.erase(std::remove_if(clips.begin(), clips.end(),
			                           [&](const AudioClip& clip)
			                           { return clip.ID == context.selectedAudioClip; }),
			            clips.end());
			context.selectedAudioClip = static_cast<id_t>(-1);
			audioRangeClip = static_cast<id_t>(-1);
			context.pushHistory("Delete Audio Clip", prev, context.score);
			warnRefreshFailure(AudioTrackUtils::refreshPlaybackAudio(context));
			return;
		}

		if (audioDragMode == AudioDragMode::None)
		{
			for (auto it = context.score.audioTrack.clips.rbegin();
			     it != context.score.audioTrack.clips.rend(); ++it)
			{
				AudioClip& clip = *it;
				if (clip.locked)
					continue;

				const ImRect rect = getAudioClipRect(context, clip);
				if (!rect.Contains(io.MousePos))
					continue;

				const float dpiScale = ImGui::GetMainViewport()->DpiScale;
				const float edgeThreshold = 8.0f * dpiScale;
				const float sourceEndMs =
				    clip.sourceEndMs >= 0.0f
				        ? clip.sourceEndMs
				        : static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
				const float clipDurationMs = std::max(1.0f, sourceEndMs - clip.sourceStartMs);
				const ImVec2 fadeInHandle(rect.Min.x + (14.0f * dpiScale),
				                          std::clamp(getScreenYFromTimelineMs(
				                                         context, clip.timelineStartMs + clip.fadeInMs),
				                                     rect.Min.y, rect.Max.y));
				const ImVec2 fadeOutHandle(rect.Max.x - (14.0f * dpiScale),
				                           std::clamp(
				                               getScreenYFromTimelineMs(
				                                   context, clip.timelineStartMs + clipDurationMs -
				                                                clip.fadeOutMs),
				                               rect.Min.y, rect.Max.y));
				const ImRect fadeInHandleRect(
				    ImVec2(fadeInHandle.x - 8.0f * dpiScale, fadeInHandle.y - 8.0f * dpiScale),
				    ImVec2(fadeInHandle.x + 8.0f * dpiScale, fadeInHandle.y + 8.0f * dpiScale));
				const ImRect fadeOutHandleRect(
				    ImVec2(fadeOutHandle.x - 8.0f * dpiScale, fadeOutHandle.y - 8.0f * dpiScale),
				    ImVec2(fadeOutHandle.x + 8.0f * dpiScale, fadeOutHandle.y + 8.0f * dpiScale));
				const float grabWidth = 16.0f * dpiScale;
				AudioDragMode hoverMode = AudioDragMode::RangeSelect;
				if (fadeInHandleRect.Contains(io.MousePos) && clipDurationMs > 1.0f)
					hoverMode = AudioDragMode::FadeIn;
				else if (fadeOutHandleRect.Contains(io.MousePos) && clipDurationMs > 1.0f)
					hoverMode = AudioDragMode::FadeOut;
				else if (std::abs(io.MousePos.y - rect.Min.y) <= edgeThreshold)
					hoverMode = AudioDragMode::TrimEnd;
				else if (std::abs(io.MousePos.y - rect.Max.y) <= edgeThreshold)
					hoverMode = AudioDragMode::TrimStart;
				else if (io.MousePos.x <= rect.Min.x + grabWidth)
					hoverMode = AudioDragMode::Move;

				if (hoverMode == AudioDragMode::Move)
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				else if (hoverMode != AudioDragMode::RangeSelect)
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					context.clearSelection();
					context.timelineEditTarget = TimelineEditTarget::Audio;
					context.selectedAudioClip = clip.ID;
					audioContextMenuClip = clip.ID;
					audioContextMenuHasRange = hasAudioRangeSelection(clip.ID);
					audioContextMenuRangeStartMs = audioRangeStartMs;
					audioContextMenuRangeEndMs = audioRangeEndMs;
					suppressTimelineContextMenu = true;
					ImGui::OpenPopup("audio_clip_context");
					break;
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					context.clearSelection();
					context.timelineEditTarget = TimelineEditTarget::Audio;
					context.selectedAudioClip = clip.ID;
					draggingAudioClip = clip.ID;
					audioDragStartClip = clip;
					audioDragStartScore = context.score;
					audioDragStartTimelineMs = currentTimelineMs;
					audioDragMode = hoverMode;
					if (audioDragMode == AudioDragMode::RangeSelect)
					{
						audioRangeClip = clip.ID;
						audioRangeStartMs = currentTimelineMs;
						audioRangeEndMs = currentTimelineMs;
					}
					else
					{
						audioRangeClip = static_cast<id_t>(-1);
					}
					break;
				}
			}
		}

		if (audioDragMode != AudioDragMode::None)
		{
			AudioClip* clip = findClip(draggingAudioClip);
			if (!clip)
			{
				audioDragMode = AudioDragMode::None;
				return;
			}

			const float deltaMs = currentTimelineMs - audioDragStartTimelineMs;
			const float sourceLengthMs =
			    static_cast<float>(context.sourceWaveformL.durationInSeconds * 1000.0);
			const float startSourceEndMs =
			    audioDragStartClip.sourceEndMs >= 0.0f ? audioDragStartClip.sourceEndMs
			                                           : sourceLengthMs;
			switch (audioDragMode)
			{
			case AudioDragMode::Move:
			{
				clip->timelineStartMs = audioDragStartClip.timelineStartMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
					clip->timelineStartMs =
					    snapTimelineMs(context, clip->timelineStartMs, true);

				const float movingDurationMs = std::max(1.0f, startSourceEndMs -
				                                             audioDragStartClip.sourceStartMs);
				const float edgeSnapThresholdMs = std::max(
				    20.0f,
				    std::abs(getTimelineMsAtScreenY(context, io.MousePos.y + 10.0f) -
				             getTimelineMsAtScreenY(context, io.MousePos.y)));
				const float candidateStartMs = clip->timelineStartMs;
				const float candidateEndMs = candidateStartMs + movingDurationMs;
				float bestStartMs = candidateStartMs;
				float bestDistanceMs = edgeSnapThresholdMs;
				auto trySnapEdge = [&](float movingEdgeMs, float targetEdgeMs, float edgeOffsetMs)
				{
					const float distance = std::abs(movingEdgeMs - targetEdgeMs);
					if (distance <= bestDistanceMs)
					{
						bestDistanceMs = distance;
						bestStartMs = targetEdgeMs - edgeOffsetMs;
					}
				};

				for (const AudioClip& otherClip : context.score.audioTrack.clips)
				{
					if (otherClip.ID == clip->ID)
						continue;

					const float otherSourceEndMs =
					    otherClip.sourceEndMs >= 0.0f
					        ? otherClip.sourceEndMs
					        : static_cast<float>(context.sourceWaveformL.durationInSeconds *
					                             1000.0);
					const float otherDurationMs =
					    std::max(1.0f, otherSourceEndMs - otherClip.sourceStartMs);
					const float otherStartMs = otherClip.timelineStartMs;
					const float otherEndMs = otherClip.timelineStartMs + otherDurationMs;

					trySnapEdge(candidateStartMs, otherStartMs, 0.0f);
					trySnapEdge(candidateStartMs, otherEndMs, 0.0f);
					trySnapEdge(candidateEndMs, otherStartMs, movingDurationMs);
					trySnapEdge(candidateEndMs, otherEndMs, movingDurationMs);
				}

				clip->timelineStartMs = std::max(0.0f, bestStartMs);
				break;
			}
			case AudioDragMode::RangeSelect:
				audioRangeEndMs = currentTimelineMs;
				break;
			case AudioDragMode::TrimStart:
			{
				float trimDelta = deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary =
					    snapTimelineMs(context, audioDragStartClip.timelineStartMs + trimDelta,
					                   true);
					trimDelta = snappedBoundary - audioDragStartClip.timelineStartMs;
				}
				const float maxTrim = std::max(0.0f, startSourceEndMs - 1.0f);
				const float newStart = std::clamp(audioDragStartClip.sourceStartMs + trimDelta,
				                                  0.0f, maxTrim);
				const float appliedTrimDelta = newStart - audioDragStartClip.sourceStartMs;
				clip->sourceStartMs = newStart;
				clip->timelineStartMs = audioDragStartClip.timelineStartMs + appliedTrimDelta;
				break;
			}
			case AudioDragMode::TrimEnd:
			{
				float newEnd = startSourceEndMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary = snapTimelineMs(
					    context,
					    audioDragStartClip.timelineStartMs +
					        (newEnd - audioDragStartClip.sourceStartMs),
					    true);
					newEnd = audioDragStartClip.sourceStartMs +
					         (snappedBoundary - audioDragStartClip.timelineStartMs);
				}
				clip->sourceEndMs =
				    std::clamp(newEnd, clip->sourceStartMs + 1.0f, sourceLengthMs);
				break;
			}
			case AudioDragMode::FadeIn:
			{
				const float durationMs =
				    std::max(1.0f, (clip->sourceEndMs >= 0.0f ? clip->sourceEndMs : sourceLengthMs) -
				                         clip->sourceStartMs);
				float fadeInMs = audioDragStartClip.fadeInMs + deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary =
					    snapTimelineMs(context, audioDragStartClip.timelineStartMs + fadeInMs,
					                   true);
					fadeInMs = snappedBoundary - audioDragStartClip.timelineStartMs;
				}
				clip->fadeInMs = std::clamp(fadeInMs, 0.0f, durationMs);
				break;
			}
			case AudioDragMode::FadeOut:
			{
				const float durationMs =
				    std::max(1.0f, (clip->sourceEndMs >= 0.0f ? clip->sourceEndMs : sourceLengthMs) -
				                         clip->sourceStartMs);
				float fadeOutMs = audioDragStartClip.fadeOutMs - deltaMs;
				if (useSnap && snapMode != SnapMode::Relative)
				{
					const float snappedBoundary = snapTimelineMs(
					    context, audioDragStartClip.timelineStartMs + durationMs - fadeOutMs,
					    true);
					fadeOutMs = audioDragStartClip.timelineStartMs + durationMs - snappedBoundary;
				}
				clip->fadeOutMs = std::clamp(fadeOutMs, 0.0f, durationMs);
				break;
			}
			default:
				break;
			}

			if (audioDragMode != AudioDragMode::RangeSelect)
			{
				context.score.audioTrack.explicitEditorTrack = true;
				if (audioDragMode == AudioDragMode::Move || audioDragMode == AudioDragMode::TrimStart)
					AudioTrackUtils::syncMusicOffsetFromAudioTrack(context);
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				if (audioDragMode != AudioDragMode::RangeSelect)
				{
					context.pushHistory("Edit Audio Clip", audioDragStartScore, context.score);
					warnRefreshFailure(AudioTrackUtils::refreshPlaybackAudio(context));
				}
				audioDragMode = AudioDragMode::None;
				draggingAudioClip = static_cast<id_t>(-1);
			}
		}
	}

	void ScoreEditorTimeline::drawWaveform(ScoreContext& context)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		constexpr ImU32 waveformColorL = 0x80646464;
		constexpr ImU32 waveformColorR = 0x80585858;

		// Ideally this should be calculated based on the current BPM
		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const double durationSeconds = context.waveformL.durationInSeconds;
		const double musicOffsetInSeconds = context.workingData.musicOffset / 1000.0f;

		const float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());

		for (size_t index = 0; index < 2; index++)
		{
			const bool rightChannel = index == 1;
			const Audio::WaveformMipChain& waveform =
			    context.isAudioEditing()
			        ? (rightChannel ? context.sourceWaveformR : context.sourceWaveformL)
			        : (rightChannel ? context.waveformR : context.waveformL);
			if (waveform.isEmpty())
				continue;

			const ImU32 waveformColor = rightChannel ? waveformColorR : waveformColorL;
			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);

			for (int y = visualOffset - size.y; y < visualOffset; y += 1)
			{
				int tick = positionToTick(y);

				// Small accuracy loss by converting to ticks but shouldn't be too noticeable
				const double secondsAtPixel =
				    accumulateDuration(tick, TICKS_PER_BEAT, context.score.tempoChanges) -
				    musicOffsetInSeconds;
				const bool outOfBounds =
				    secondsAtPixel < 0 || secondsAtPixel > waveform.durationInSeconds;

				float amplitude =
				    std::max(waveform.getAmplitudeAt(mip, secondsAtPixel, secondsPerPixel), 0.0f);
				float barValue = outOfBounds ? 0.0f : (amplitude * std::min(laneWidth * 6, 180.0f));
				float rectYPosition = floorf(position.y + visualOffset - y);
				// WARNING: A thickness of 0.5 or less does not draw with integrated graphics
				// (optimization? limitation?)

				float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());
				ImVec2 rect1(timelineMidPosition, rectYPosition);
				ImVec2 rect2(timelineMidPosition +
				                 (std::max(0.75f, barValue) * (rightChannel ? 1 : -1)),
				             rectYPosition + 0.75f);
				drawList->AddRectFilled(rect1, rect2, waveformColor);
			}
		}
	}

	void ScoreEditorTimeline::scrollTimeline(ScoreContext& context, const int tick)
	{
		context.currentTick = tick;
		offset = std::max(minOffset, tickToPosition(context.currentTick) +
		                                 (size.y * (1.0f - config.cursorPositionThreshold)));
	}
	
	void ScoreEditorTimeline::drawHiSpeedGraph(ScoreContext& context)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (!drawList) return;

	std::map<int, std::vector<HiSpeedChange>> layerChanges;
	for (const auto& [id, hiSpeed] : context.score.hiSpeedChanges)
	{
		if (context.score.layers[hiSpeed.layer].hidden)
			continue;
		layerChanges[hiSpeed.layer].push_back(hiSpeed);
	}

	if (layerChanges.empty()) return;

	for (auto& [layer, changes] : layerChanges) {
		std::sort(changes.begin(), changes.end(), [](const HiSpeedChange& a, const HiSpeedChange& b) {
			return a.tick < b.tick;
		});
	}

	float dpiScale = ImGui::GetMainViewport()->DpiScale;
	const ImRect hiSpeedRect = getHiSpeedDisplayRect(context);
	float laneStartX = hiSpeedRect.Min.x;
	float laneWidth = hiSpeedRect.Max.x - hiSpeedRect.Min.x;
	float laneEndX = hiSpeedRect.Max.x;
	if (laneWidth <= 20.0f * dpiScale)
		return;
	
	// configの値 (0.0~1.0) を 0~255 に変換して適用
	int bgAlpha = std::clamp((int)(config.hiSpeedGraphBgOpacity * 255.0f), 0, 255);
	ImU32 bgColor = IM_COL32(31, 26, 28, bgAlpha);
	drawList->AddRectFilled(ImVec2(laneStartX, position.y), ImVec2(laneEndX, position.y + size.y), bgColor);

	float padding = 15.0f * dpiScale;
	float drawableStartX = laneStartX + padding;
	float drawableWidth = laneWidth - (padding * 2);
	if (drawableWidth <= 1.0f)
		return;

	// 設定画面で指定された上限・下限値を参照する
	float graphLimit = config.hiSpeedGraphLimit;

	// スケール計算（余計なマージンを廃止し、上限に達したらピッタリ壁に付くように修正）
	auto calcScale = [&](const std::vector<HiSpeedChange>& changes) {
		float minS = 0.0f;
		float maxS = 1.0f; // 最低でも 1.0x までは描画領域を確保
		for (const auto& change : changes) {
			float s = std::clamp(change.speed, -graphLimit, graphLimit);
			if (s < minS) minS = s;
			if (s > maxS) maxS = s;
		}
		// 0.0x の基準線が左壁にベタ付きにならないよう、マイナス側にだけ 25% の余白を設ける
		// プラス側（右壁）には余白を足さないため、上限値はパディング位置にピッタリ張り付く
		float requiredMin = -maxS * 0.25f;
		if (minS > requiredMin) {
			minS = requiredMin;
		}
		return std::make_pair(minS, maxS);
	};

	auto getX = [&](float speed, float minS, float maxS) {
		float normalized = (speed - minS) / (maxS - minS);
		return drawableStartX + (normalized * drawableWidth);
	};

	float activeMin = 0.0f, activeMax = 1.0f;
	if (layerChanges.find(context.selectedLayer) != layerChanges.end()) {
		auto scale = calcScale(layerChanges[context.selectedLayer]);
		activeMin = scale.first;
		activeMax = scale.second;
	} else {
		std::vector<HiSpeedChange> dummy;
		auto scale = calcScale(dummy);
		activeMin = scale.first;
		activeMax = scale.second;
	}

	float baselineX = getX(0.0f, activeMin, activeMax);
	ImU32 baselineColor = IM_COL32(255, 255, 255, 60);
	drawList->AddLine(ImVec2(baselineX, position.y), ImVec2(baselineX, position.y + size.y), baselineColor, 1.0f);

	ImVec2 mousePos = ImGui::GetMousePos();
	bool isAnyNodeHovered = false;

	ImU32 ghostLineColor = IM_COL32(150, 150, 150, 60);
	ImU32 ghostPointColor = IM_COL32(150, 150, 150, 60);

	for (const auto& [layer, changes] : layerChanges) {
		if (layer == context.selectedLayer) continue;

		auto ghostScale = calcScale(changes);
		float gMin = ghostScale.first;
		float gMax = ghostScale.second;

		for (size_t i = 0; i < changes.size(); ++i) {
			const auto& current = changes[i];
			
			// 大きさと色の判定
			bool isLarge = (current.speed >= graphLimit || current.speed <= -graphLimit);
			bool isUltraExtreme = (current.speed >= 1000.0f || current.speed <= -1000.0f);
			
			float cSpeed1 = std::clamp(current.speed, -graphLimit, graphLimit);
			float y1 = position.y - tickToPosition(current.tick) + visualOffset;
			float x1 = getX(cSpeed1, gMin, gMax);

			if (i < changes.size() - 1) {
				const auto& next = changes[i + 1];
				float cSpeed2 = std::clamp(next.speed, -graphLimit, graphLimit);
				float y2 = position.y - tickToPosition(next.tick) + visualOffset;
				float x2 = getX(cSpeed2, gMin, gMax);

				if (current.ease == HiSpeedEaseType::Linear) {
					drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ghostLineColor, 1.5f);
				} else {
					drawList->AddLine(ImVec2(x1, y1), ImVec2(x1, y2), ghostLineColor, 1.5f);
					drawList->AddLine(ImVec2(x1, y2), ImVec2(x2, y2), ghostLineColor, 1.5f);
				}
			} else {
				drawList->AddLine(ImVec2(x1, y1), ImVec2(x1, position.y), ghostLineColor, 1.5f);
			}
			
			float radius = isLarge ? 3.5f : 2.0f;
			ImU32 pColor = isUltraExtreme ? IM_COL32(255, 120, 120, 80) : ghostPointColor;
			drawList->AddCircleFilled(ImVec2(x1, y1), radius, pColor);
		}
	}

	ImU32 lineColor = speedColor;
	ImU32 pointColor = IM_COL32(255, 255, 255, 255); 
	ImU32 hoverColor = IM_COL32(255, 150, 150, 255); 
	ImU32 selectedColor = IM_COL32(255, 200, 100, 255); 

	static id_t draggingNodeID = -1;
	static bool nodeWasDragged = false;
	static Score prevScoreForDrag;
	static float dragAccumulatedSpeed = 0.0f;

	const bool hiSpeedEditable = context.isNotesEditing();
	if (!hiSpeedEditable)
	{
		draggingNodeID = -1;
		nodeWasDragged = false;
	}

	std::vector<HiSpeedChange> hoveredTooltips;

	if (layerChanges.find(context.selectedLayer) != layerChanges.end()) {
		auto& activeChanges = layerChanges[context.selectedLayer];
		
		for (size_t i = 0; i < activeChanges.size(); ++i)
		{
			const auto& current = activeChanges[i];
			
			// 大きさと色の判定（アクティブレイヤーも確実に graphLimit に連動）
			bool isLarge = (current.speed >= graphLimit || current.speed <= -graphLimit);
			bool isUltraExtreme = (current.speed >= 1000.0f || current.speed <= -1000.0f);
			
			float cSpeed1 = std::clamp(current.speed, -graphLimit, graphLimit);
			float y1 = position.y - tickToPosition(current.tick) + visualOffset;
			float x1 = getX(cSpeed1, activeMin, activeMax);

			if (i < activeChanges.size() - 1)
			{
				const auto& next = activeChanges[i + 1];
				float cSpeed2 = std::clamp(next.speed, -graphLimit, graphLimit);
				float y2 = position.y - tickToPosition(next.tick) + visualOffset;
				float x2 = getX(cSpeed2, activeMin, activeMax);

				if (current.ease == HiSpeedEaseType::Linear) {
					drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineColor, 2.0f);
				} else {
					drawList->AddLine(ImVec2(x1, y1), ImVec2(x1, y2), lineColor, 2.0f);
					drawList->AddLine(ImVec2(x1, y2), ImVec2(x2, y2), lineColor, 2.0f);
				}
			}
			else
			{
				drawList->AddLine(ImVec2(x1, y1), ImVec2(x1, position.y), lineColor, 2.0f);
			}

			bool isSelected = context.selectedHiSpeedChanges.find(current.ID) != context.selectedHiSpeedChanges.end();
			bool isHovered = false;
			bool isDraggingThis = (draggingNodeID == current.ID);

			if (abs(mousePos.x - x1) < 8.0f && abs(mousePos.y - y1) < 8.0f) {
				isHovered = true;
				isAnyNodeHovered = true;
			}

			if (hiSpeedEditable && (isHovered || isDraggingThis)) {
				isHoveringNote = true; 
			}

			ImU32 extremeNormalColor = IM_COL32(255, 140, 140, 255); 
			ImU32 extremeHoverColor  = IM_COL32(255, 180, 180, 255);
			ImU32 extremeSelectColor = IM_COL32(255, 150, 50, 255);

			// 上限に達していたら大きくする
			float radius = (isHovered || isDraggingThis) ? 5.0f : 3.0f;
			if (isLarge) radius += 2.0f;

			ImU32 circleColor;
			// 1000.0x以上なら色を赤っぽくする
			if (isUltraExtreme) {
				circleColor = isSelected ? extremeSelectColor : ((isHovered || isDraggingThis) ? extremeHoverColor : extremeNormalColor);
			} else {
				circleColor = isSelected ? selectedColor : ((isHovered || isDraggingThis) ? hoverColor : pointColor);
			}
			drawList->AddCircleFilled(ImVec2(x1, y1), radius, circleColor);

			if (hiSpeedEditable && isHovered && draggingNodeID == -1) {
				hoveredTooltips.push_back(current);

				if (ImGui::IsMouseClicked(0)) {
					if (ImGui::GetIO().KeyCtrl) {
						if (isSelected) {
							context.selectedHiSpeedChanges.erase(current.ID);
						} else {
							context.selectedHiSpeedChanges.insert(current.ID);
						}
					} else {
						if (!isSelected) {
							context.selectedNotes.clear();
							context.selectedHiSpeedChanges.clear();
							context.selectedMetaEvents.clear();
							context.selectedHiSpeedChanges.insert(current.ID);
						}
					}

					draggingNodeID = current.ID;
					nodeWasDragged = false;
					prevScoreForDrag = context.score;
					dragAccumulatedSpeed = current.speed;
				}

				if (ImGui::IsMouseClicked(1)) {
					if (current.tick != 0) {
						Score prev = context.score;
						
						std::unordered_set<id_t> deleteIds;
						if (context.selectedHiSpeedChanges.find(current.ID) != context.selectedHiSpeedChanges.end()) {
							deleteIds = context.selectedHiSpeedChanges;
						} else {
							deleteIds.insert(current.ID);
						}

						for (auto id : deleteIds) {
							if (context.score.hiSpeedChanges[id].tick != 0) { 
								context.score.hiSpeedChanges.erase(id);
							}
						}
						context.pushHistory("Delete hi-speed", prev, context.score);
					}
				}
			}

			if (hiSpeedEditable && isDraggingThis) {
				if (ImGui::IsMouseDragging(0, 2.0f)) {
					nodeWasDragged = true;
				}

				if (nodeWasDragged) {
					float speedPerPixel = (activeMax - activeMin) / drawableWidth;
					if (speedPerPixel < 0.01f) speedPerPixel = 0.01f;

					dragAccumulatedSpeed += ImGui::GetIO().MouseDelta.x * speedPerPixel;
					
					float newSpeed = std::round(dragAccumulatedSpeed * 4.0f) / 4.0f;

					int originalTick = prevScoreForDrag.hiSpeedChanges[current.ID].tick;
					float originalSpeed = prevScoreForDrag.hiSpeedChanges[current.ID].speed;

					int newTick = std::max(0, hoverTick);
					if (originalTick == 0) newTick = 0; 

					int tickDelta = newTick - originalTick;
					float speedDelta = newSpeed - originalSpeed;

					if (ImGui::GetIO().KeyShift) {
						speedDelta = 0.0f;
					}

					std::unordered_set<id_t> moveIds;
					if (context.selectedHiSpeedChanges.find(current.ID) != context.selectedHiSpeedChanges.end()) {
						moveIds = context.selectedHiSpeedChanges;
					} else {
						moveIds.insert(current.ID);
					}

					for (auto id : moveIds) {
						if (prevScoreForDrag.hiSpeedChanges[id].tick + tickDelta < 0) {
							tickDelta = -prevScoreForDrag.hiSpeedChanges[id].tick;
						}
					}

					for (auto id : moveIds) {
						if (prevScoreForDrag.hiSpeedChanges[id].tick == 0) {
							tickDelta = 0;
						}
					}

					bool canMoveTime = true;
					if (tickDelta != 0) {
						for (auto id : moveIds) {
							int targetT = prevScoreForDrag.hiSpeedChanges[id].tick + tickDelta;
							int targetL = prevScoreForDrag.hiSpeedChanges[id].layer;
							for (const auto& [otherId, hsc] : prevScoreForDrag.hiSpeedChanges) {
								if (moveIds.find(otherId) == moveIds.end() && hsc.tick == targetT && hsc.layer == targetL) {
									canMoveTime = false; break;
								}
							}
							if (!canMoveTime) break;
						}
					} else {
						canMoveTime = true;
					}

					if (!canMoveTime) tickDelta = 0; 

					for (auto id : moveIds) {
						auto& editingNode = context.score.hiSpeedChanges[id];
						const auto& prevNode = prevScoreForDrag.hiSpeedChanges[id];
						editingNode.tick = prevNode.tick + tickDelta;
						editingNode.speed = prevNode.speed + speedDelta;
					}

					ImGui::BeginTooltip();
					ImGui::Text("Speed: %.2fx", context.score.hiSpeedChanges[current.ID].speed);
					ImGui::Text("Tick: %d", context.score.hiSpeedChanges[current.ID].tick);
					if (moveIds.size() > 1) {
						ImGui::Text("(%zu items moving)", moveIds.size());
					}
					ImGui::EndTooltip();
				}

				if (ImGui::IsMouseReleased(0)) {
					if (nodeWasDragged) {
						context.pushHistory("Drag hi-speed node", prevScoreForDrag, context.score);
					} else {
						if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
							eventEdit.editId = current.ID;
							eventEdit.editHiSpeed = current.speed;
							eventEdit.editHiSpeedEase = current.ease;
							eventEdit.editHiSpeedSkip = current.skips;
							eventEdit.editHiSpeedHideNote = current.hideNotes;
							eventEdit.type = EventType::HiSpeed;
							ImGui::OpenPopup("edit_event");
						}
					}
					draggingNodeID = -1;
				}
			}
		}
	}

	if (hiSpeedEditable && !hoveredTooltips.empty() && draggingNodeID == -1) {
		ImGui::BeginTooltip();
		for (const auto& node : hoveredTooltips) {
			ImGui::Text("%.2fx", node.speed);
			ImGui::SameLine();
			ImGui::TextDisabled("Tick: %d", node.tick);
			if (node.skips > 0.0f) {
				ImGui::SameLine();
				ImGui::TextDisabled("| Skip: %.2f", node.skips);
			}
		}
		ImGui::EndTooltip();
	}

	if (hiSpeedEditable && !isAnyNodeHovered && mouseInTimeline && mousePos.x >= laneStartX && mousePos.x <= laneEndX) {
		if (ImGui::IsMouseDoubleClicked(0)) {
			float normalizedX = std::clamp((mousePos.x - drawableStartX) / drawableWidth, 0.0f, 1.0f);
			float newSpeed = activeMin + (normalizedX * (activeMax - activeMin));
			newSpeed = std::round(newSpeed * 4.0f) / 4.0f;

			bool canAdd = true;
			for (const auto& [id, hsc] : context.score.hiSpeedChanges) {
				if (hsc.tick == hoverTick && hsc.layer == context.selectedLayer) {
					canAdd = false; break;
				}
			}

			if (canAdd) {
				Score prev = context.score;
				id_t newId = getNextHiSpeedID();
				context.score.hiSpeedChanges[newId] = {
					newId,
					hoverTick,
					newSpeed,
					context.selectedLayer,
					0.0f, 
					HiSpeedEaseType::None,
					false
				};
				context.pushHistory("Add hi-speed", prev, context.score);
			}
		}
	}
}
}
