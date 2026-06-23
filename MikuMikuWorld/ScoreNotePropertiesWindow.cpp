#include "ScoreEditorWindows.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "NoteTypes.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <stdexcept>

namespace MikuMikuWorld
{
	void ScoreNotePropertiesWindow::update(ScoreContext& context, int currentDivision)
	{
		auto numSelected = context.selectedNotes.size() + context.selectedHiSpeedChanges.size() +
		                   context.selectedMetaEvents.size();
		if (numSelected == 0)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		Score prev = context.score;
		bool edited = false;

		if (!context.selectedMetaEvents.empty() && context.selectedNotes.empty() &&
		    context.selectedHiSpeedChanges.empty())
		{
			if (context.selectedMetaEvents.size() > 1)
			{
				ImGui::Text("%s", "Multiple meta events selected.");
				return;
			}

			const SelectedMetaEvent selectedMeta = *context.selectedMetaEvents.begin();
			int selectedTick = context.getMetaEventTick(selectedMeta);
			if (selectedTick < 0)
			{
				ImGui::Text("%s", getString("note_properties_not_selected"));
				return;
			}

			if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG, getString("general"), " ").c_str(),
			                            ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				double beat = selectedTick / static_cast<float>(TICKS_PER_BEAT);
				UI::propertyLabel(getString("beat"));
				ImGui::SetNextItemWidth(-1);
				double step = 4.0 / (currentDivision > 0 ? currentDivision : 4);
				double step_fast = step * 4.0;
				bool beatChanged =
				    ImGui::InputDouble(IO::concat("##", getString("beat")).c_str(), &beat, step,
				                       step_fast, "%.3f");
				ImGui::NextColumn();

				if (beatChanged)
				{
					int newTick = std::max(0, static_cast<int>(std::round(beat * TICKS_PER_BEAT)));
					if (selectedMeta.kind == MetaEventKind::Waypoint)
					{
						for (Waypoint& waypoint : context.score.waypoints)
							if (waypoint.runtimeId == selectedMeta.key)
								waypoint.tick = newTick;
						edited = true;
					}
					else if (selectedMeta.kind == MetaEventKind::Bpm)
					{
						bool duplicate = false;
						for (const Tempo& tempo : context.score.tempoChanges)
							if (tempo.runtimeId != selectedMeta.key && tempo.tick == newTick)
								duplicate = true;
						if (!duplicate)
						{
							for (Tempo& tempo : context.score.tempoChanges)
								if (tempo.runtimeId == selectedMeta.key && tempo.tick != 0)
									tempo.tick = newTick;
							std::sort(context.score.tempoChanges.begin(),
							          context.score.tempoChanges.end(),
							          [](const Tempo& a, const Tempo& b)
							          { return a.tick < b.tick; });
							edited = true;
						}
					}
					else if (selectedMeta.kind == MetaEventKind::Skill)
					{
						if (context.score.skills.find(selectedMeta.key) !=
						    context.score.skills.end())
						{
							context.score.skills[selectedMeta.key].tick = newTick;
							edited = true;
						}
					}
					else if (selectedMeta.kind == MetaEventKind::FeverStart)
					{
						context.score.fever.startTick =
						    std::min(newTick, context.score.fever.endTick);
						edited = true;
					}
					else if (selectedMeta.kind == MetaEventKind::FeverEnd)
					{
						context.score.fever.endTick =
						    std::max(newTick, context.score.fever.startTick);
						edited = true;
					}
				}

				if (config.showTickInProperties)
				{
					if (UI::addIntProperty(getString("tick"), selectedTick))
					{
						selectedTick = std::max(0, selectedTick);
						if (selectedMeta.kind == MetaEventKind::Waypoint)
						{
							for (Waypoint& waypoint : context.score.waypoints)
								if (waypoint.runtimeId == selectedMeta.key)
									waypoint.tick = selectedTick;
							edited = true;
						}
						else if (selectedMeta.kind == MetaEventKind::Bpm)
						{
							bool duplicate = false;
							for (const Tempo& tempo : context.score.tempoChanges)
								if (tempo.runtimeId != selectedMeta.key &&
								    tempo.tick == selectedTick)
									duplicate = true;
							if (!duplicate)
							{
								for (Tempo& tempo : context.score.tempoChanges)
									if (tempo.runtimeId == selectedMeta.key && tempo.tick != 0)
										tempo.tick = selectedTick;
								std::sort(context.score.tempoChanges.begin(),
								          context.score.tempoChanges.end(),
								          [](const Tempo& a, const Tempo& b)
								          { return a.tick < b.tick; });
								edited = true;
							}
						}
						else if (selectedMeta.kind == MetaEventKind::Skill)
						{
							if (context.score.skills.find(selectedMeta.key) !=
							    context.score.skills.end())
							{
								context.score.skills[selectedMeta.key].tick = selectedTick;
								edited = true;
							}
						}
					}
				}
				UI::endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(getString("meta_event"),
			                            ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				switch (selectedMeta.kind)
				{
				case MetaEventKind::Waypoint:
					for (Waypoint& waypoint : context.score.waypoints)
					{
						if (waypoint.runtimeId != selectedMeta.key)
							continue;
						UI::addStringProperty(getString("waypoint_name"), waypoint.name);
						if (ImGui::IsItemDeactivatedAfterEdit())
							edited = true;
					}
					break;
				case MetaEventKind::Bpm:
					for (Tempo& tempo : context.score.tempoChanges)
					{
						if (tempo.runtimeId != selectedMeta.key)
							continue;
						if (UI::addFloatProperty(getString("bpm"), tempo.bpm, "%g"))
						{
							tempo.bpm = std::clamp(tempo.bpm, static_cast<float>(MIN_BPM),
							                       static_cast<float>(MAX_BPM));
							edited = true;
						}
					}
					break;
				case MetaEventKind::TimeSignature:
					if (context.score.timeSignatures.find((int)selectedMeta.key) !=
					    context.score.timeSignatures.end())
					{
						TimeSignature& ts = context.score.timeSignatures[(int)selectedMeta.key];
						int measure = ts.measure;
						if (UI::addIntProperty(getString("measure"), measure, 0, INT_MAX))
						{
							if (measure != ts.measure &&
							    context.score.timeSignatures.find(measure) ==
							        context.score.timeSignatures.end())
							{
								TimeSignature moved = ts;
								context.score.timeSignatures.erase(ts.measure);
								moved.measure = measure;
								context.score.timeSignatures[measure] = moved;
								context.selectedMetaEvents.clear();
								context.selectedMetaEvents.insert(
								    { MetaEventKind::TimeSignature, static_cast<id_t>(measure) });
								edited = true;
							}
						}
						TimeSignature& updatedTs =
						    context.score.timeSignatures[(int)context.selectedMetaEvents.begin()->key];
						if (UI::timeSignatureSelect(updatedTs.numerator, updatedTs.denominator))
						{
							updatedTs.numerator =
							    std::clamp(abs(updatedTs.numerator), MIN_TIME_SIGNATURE,
							               MAX_TIME_SIGNATURE_NUMERATOR);
							updatedTs.denominator =
							    std::clamp(abs(updatedTs.denominator), MIN_TIME_SIGNATURE,
							               MAX_TIME_SIGNATURE_DENOMINATOR);
							edited = true;
						}
					}
					break;
				case MetaEventKind::Skill:
					if (context.score.skills.find(selectedMeta.key) != context.score.skills.end())
					{
						SkillTrigger& skill = context.score.skills[selectedMeta.key];
						int level = skill.level;
						edited |= UI::addSelectProperty(getString("skill_effect"), skill.effect,
						                                skillEffectTypes,
						                                arrayLength(skillEffectTypes));
						if (UI::addIntProperty(getString("skill_level"), level, "Lv.%d", 1, 4))
						{
							skill.level = static_cast<uint8_t>(std::clamp(level, 1, 4));
							edited = true;
						}
					}
					break;
				case MetaEventKind::FeverStart:
				case MetaEventKind::FeverEnd:
					edited |= UI::addIntProperty(getString("fever_start_tick"),
					                             context.score.fever.startTick, -1, INT_MAX);
					edited |= UI::addIntProperty(getString("fever_end_tick"),
					                             context.score.fever.endTick, -1, INT_MAX);
					if (context.score.fever.startTick >= 0 && context.score.fever.endTick >= 0 &&
					    context.score.fever.endTick < context.score.fever.startTick)
					{
						context.score.fever.endTick = context.score.fever.startTick;
					}
					break;
				}
				UI::endPropertyColumns();
			}

			if (edited)
				context.pushHistory("Edited meta event", prev, context.score);
			return;
		}

		int selectedTick;
		int selectedLayer;
		try
		{
			selectedTick =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).tick
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin()).tick;
			selectedLayer =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).layer
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin())
			              .layer;
		}
		catch (const std::out_of_range& e)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG, getString("general"), " ").c_str(),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();

			double beat = selectedTick / static_cast<float>(TICKS_PER_BEAT);
			
			UI::propertyLabel(getString("beat"));
			ImGui::SetNextItemWidth(-1);

			double step = 4.0 / (currentDivision > 0 ? currentDivision : 4);
			double step_fast = step * 4.0;
			
			bool beatChanged = ImGui::InputDouble(IO::concat("##", getString("beat")).c_str(), &beat, step, step_fast, "%.3f");
			ImGui::NextColumn();

			if (beatChanged)
			{
				auto newTick = std::round(beat * TICKS_PER_BEAT); 
				for (auto& id : context.selectedNotes)
				{
					context.score.notes.at(id).tick = newTick;
				}
				for (auto& id : context.selectedHiSpeedChanges)
				{
					context.score.hiSpeedChanges.at(id).tick = newTick;
				}
				edited = true;
			}

			if (config.showTickInProperties)
			{
				if (UI::addIntProperty(getString("tick"), selectedTick))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).tick = selectedTick;
					}
					for (auto& id : context.selectedHiSpeedChanges)
					{
						context.score.hiSpeedChanges.at(id).tick = selectedTick;
					}
					edited = true;
				}
			}

			UI::propertyLabel(getString("layer"));
			const std::string layer_name = context.score.layers[selectedLayer].name;
			if (ImGui::BeginCombo(IO::concat("##", getString("layer")).c_str(), layer_name.c_str()))
			{
				for (int i = 0; i < context.score.layers.size(); i++)
				{
					auto& layer = context.score.layers[i];
					bool selected = selectedLayer == i;
					if (ImGui::Selectable(layer.name.c_str(), selected))
					{
						for (auto& id : context.selectedNotes)
						{
							context.score.notes.at(id).layer = i;
						}
						for (auto& id : context.selectedHiSpeedChanges)
						{
							context.score.hiSpeedChanges.at(id).layer = i;
						}
						edited = true;
					}
				}
				ImGui::EndCombo();
			}

			UI::endPropertyColumns();
		}

		if (context.selectedNotes.size() >= 1)
		{
			bool isGuide = false;

			bool multipleHold = false;
			int holdIndex = -1;
			for (id_t id : context.selectedNotes)
			{
				const Note& n = context.score.notes.at(id);
				if (n.isHold())
				{
					auto prevHoldIndex = holdIndex;
					if (n.getType() == NoteType::Hold)
					{
						holdIndex = id;
					}
					else
					{
						holdIndex = n.parentID;
					}
					if (prevHoldIndex != -1 && prevHoldIndex != holdIndex)
					{
						multipleHold = true;
						break;
					}
				}
			}

			Note& note = context.score.notes.at(*context.selectedNotes.begin());
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_COG, getString("note_properties_note"), " ").c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				if (UI::addFloatProperty(getString("lane"), note.lane, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).lane = note.lane;
					}
					edited = true;
				}

				if (UI::addFloatProperty(getString("width"), note.width, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& localNote = context.score.notes.at(id);
						if (localNote.isHold())
						{
							auto& hold = context.score.holdNotes.at(
							    localNote.parentID == -1 ? id : localNote.parentID);
							if (!((localNote.getType() == NoteType::Hold &&
							       hold.startType == HoldNoteType::Normal) ||
							      (localNote.getType() == NoteType::HoldMid &&
							       hold.steps.at(findHoldStep(hold, localNote.ID)).type ==
							           HoldStepType::Normal) ||
							      (localNote.getType() == NoteType::HoldEnd &&
							       hold.endType == HoldNoteType::Normal)))
							{
								localNote.width = std::max(0.0f, note.width);
								continue;
							}
						}

						localNote.width = std::max(0.5f, note.width);
					}
					edited = true;
				}
			}

			if (context.hasHoldInSelection())
			{
				if (!multipleHold)
				{
					auto& hold = context.score.holdNotes.at(holdIndex);
					auto& holdStart = context.score.notes.at(hold.start.ID);

					isGuide = hold.isGuide();
					if (note.width == 0 && !isGuide)
					{
						if (note.getType() == NoteType::Hold)
						{
							hold.startType = HoldNoteType::Hidden;
						}
						else if (note.getType() == NoteType::HoldEnd)
						{
							hold.endType = HoldNoteType::Hidden;
						}
					}

					if (!isGuide)
					{
						if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
						{
							if (UI::addCheckboxProperty(getString("trace"), note.friction))
							{
								for (auto& id : context.selectedNotes)
								{
									auto& n = context.score.notes.at(id);
									if (n.canTrace())
									{
										n.friction = note.friction;
									}
								}
								edited = true;
							}
						}
						if (UI::addCheckboxProperty(getString("critical"), note.critical))
						{
							context.score.notes.at(hold.start.ID).critical = note.critical;
							for (auto& step : hold.steps)
							{
								context.score.notes.at(step.ID).critical = note.critical;
							}
							context.score.notes.at(hold.end).critical = note.critical;
							for (auto& id : context.selectedNotes)
							{
								context.score.notes.at(id).critical = note.critical;
							}
							edited = true;
						}

						bool selectingAnyDummy = !std::all_of(
						    context.selectedNotes.begin(), context.selectedNotes.end(),
						    [&](id_t id)
						    {
							    auto& n = context.score.notes.at(id);
							    if (!n.isHold())
								    return false;
							    auto& h = context.score.holdNotes.at(
							        n.getType() == NoteType::Hold ? n.ID : n.parentID);
							    switch (n.getType())
							    {
							    case NoteType::Hold:
								    return h.startType == HoldNoteType::Hidden;
							    case NoteType::HoldEnd:
								    return h.endType == HoldNoteType::Hidden;
							    case NoteType::HoldMid:
								    return h[findHoldStep(h, id)].type == HoldStepType::Hidden;
							    }
							    return false;
						    });

						if (selectingAnyDummy &&
						    UI::addCheckboxProperty(getString("dummy"), note.dummy))
						{
							for (auto& id : context.selectedNotes)
							{
								auto& n = context.score.notes.at(id);
								n.dummy = note.dummy;
								if (n.dummy)
									n.soundEffect = SoundEffectType::Default;
							}
							edited = true;
						}
					}

					if (note.getType() == NoteType::HoldEnd && !isGuide)
					{
						if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick,
						                                       flickTypes, arrayLength(flickTypes)))
						{
							context.score.notes.at(hold.start.ID).flick = note.flick;
							for (auto& id : context.selectedNotes)
							{
								auto& n = context.score.notes.at(id);
								if (!n.isHold())
								{
									n.flick = note.flick;
								}
							}
						}
					}
				}
			}
			else
			{
				if (UI::addCheckboxProperty(getString("trace"), note.friction))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canTrace())
						{
							n.friction = note.friction;
						}
					}
					edited = true;
				}
				if (UI::addCheckboxProperty(getString("critical"), note.critical))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).critical = note.critical;
					}
					edited = true;
				}
				if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick, flickTypes,
				                                       arrayLength(flickTypes)))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canFlick())
						{
							n.flick = note.flick;
						}
					}
					edited = true;
				}

				if (UI::addCheckboxProperty(getString("dummy"), note.dummy))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						n.dummy = note.dummy;
						if (n.dummy)
							n.soundEffect = SoundEffectType::Default;
					}
					edited = true;
				}
			}

			bool hasEditableSoundEffect = false;
			SoundEffectType soundEffect = note.soundEffect;
			for (auto id : context.selectedNotes)
			{
				auto& n = context.score.notes.at(id);
				if (!n.canSoundEffect())
					continue;
				if (!hasEditableSoundEffect)
					soundEffect = n.soundEffect;
				hasEditableSoundEffect = true;
			}

			if (hasEditableSoundEffect &&
			    UI::addSelectProperty(getString("sound_effect"), soundEffect,
			                          soundEffectTypes, arrayLength(soundEffectTypes)))
			{
				for (auto& id : context.selectedNotes)
				{
					auto& n = context.score.notes.at(id);
					if (n.canSoundEffect())
						n.soundEffect = soundEffect;
				}
				edited = true;
			}

			UI::endPropertyColumns();
			if (context.hasHoldInSelection())
			{
				if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG,
				                                       isGuide
				                                           ? getString("note_properties_guide")
				                                           : getString("note_properties_hold_note"),
				                                       " ")
				                                .c_str(),
				                            ImGuiTreeNodeFlags_DefaultOpen))
				{

					if (multipleHold)
					{
						ImGui::Text("%s", getString("note_properties_many_hold_notes"));
					}
					else
					{
						auto& hold = context.score.holdNotes.at(holdIndex);
						auto& holdStart = context.score.notes.at(hold.start.ID);

						int stepIndex = findHoldStep(hold, note.ID);

						UI::beginPropertyColumns();

						bool hasEaseable = false;
						Note easeableNote;
						bool hasStepType = false;
						Note stepTypeNote;
						bool hasStepLayer = false;
						Note stepLayerNote;
						for (auto id : context.selectedNotes)
						{
							auto& n = context.score.notes.at(id);
							if (n.hasEase())
							{
								if (!hasEaseable)
								{
									easeableNote = n;
								}
								hasEaseable = true;
							}
							if (context.score.notes.at(id).getType() == NoteType::HoldMid)
							{
								if (!hasStepType)
								{
									stepTypeNote = n;
								}
								hasStepType = true;
							}
							if (n.getType() == NoteType::Hold || n.getType() == NoteType::HoldMid)
							{
								if (!hasStepLayer)
								{
									stepLayerNote = n;
								}
								hasStepLayer = true;
							}
						}

						if (hasEaseable)
						{
							int stepIndex = findHoldStep(hold, easeableNote.ID);
							auto ease = easeableNote.getType() == NoteType::Hold
							                ? hold.start.ease
							                : hold.steps.at(stepIndex == -1 ? 0 : stepIndex).ease;
							if (UI::addSelectProperty(getString("ease_type"), ease, easeTypes,
							                          arrayLength(easeTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.hasEase())
									{
										if (note.getType() == NoteType::Hold)
										{
											hold.start.ease = ease;
										}
										else
										{
											auto& step = hold.steps.at(findHoldStep(hold, note.ID));
											step.ease = ease;
										}
									}
								}
							}
						}

						if (hasStepType)
						{
							int stepIndex = findHoldStep(hold, stepTypeNote.ID);
							auto stepType = hold.steps.at(stepIndex).type;

							if (UI::addSelectProperty(getString("step_type"), stepType, stepTypes,
							                          arrayLength(stepTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::HoldMid)
									{
										auto& step = hold.steps.at(findHoldStep(hold, note.ID));
										step.type = stepType;
									}
								}
							}
						}

						if (hasStepLayer)
						{
							HoldStepLayer stepLayer = HoldStepLayer::Top;
							if (stepLayerNote.getType() == NoteType::Hold)
							{
								stepLayer = hold.start.layer;
							}
							else
							{
								int stepIndex = findHoldStep(hold, stepLayerNote.ID);
								if (stepIndex != -1)
								{
									stepLayer = hold.steps.at(stepIndex).layer;
								}
							}

							if (UI::addSelectProperty(getString("hold_step_layer"), stepLayer,
							                          holdStepLayers, arrayLength(holdStepLayers)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::Hold)
									{
										hold.start.layer = stepLayer;
									}
									else if (note.getType() == NoteType::HoldMid)
									{
										int localStepIndex = findHoldStep(hold, note.ID);
										if (localStepIndex != -1)
										{
											hold.steps.at(localStepIndex).layer = stepLayer;
										}
									}
								}
								edited = true;
							}
						}

						if (isGuide)
						{
							edited |= UI::addSelectProperty(getString("guide_color"),
							                                hold.guideColor, guideColorsForString,
							                                arrayLength(guideColors));
							edited |= UI::addSelectProperty(getString("fade_type"), hold.fadeType,
							                                fadeTypes, arrayLength(fadeTypes));
						}
						else
						{
							auto holdType =
							    note.getType() == NoteType::Hold ? hold.startType : hold.endType;
							if (UI::addSelectProperty(getString("hold_type"), holdType, holdTypes,
							                          2))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::Hold)
									{
										hold.startType = holdType;
									}
									else
									{
										hold.endType = holdType;
									}
								}
								edited = true;
							}

							if (UI::addCheckboxProperty(getString("hold_dummy"), hold.dummy))
							{
								edited = true;
							}
						}
					}
				}

				UI::endPropertyColumns();
			}
		}
		if (context.selectedHiSpeedChanges.size() >= 1)
		{
			bool speedEdited = false;
			HiSpeedChange& editHiSpeed =
			    context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin());
			float speed = editHiSpeed.speed;
			float skip = editHiSpeed.skips;
			auto ease = editHiSpeed.ease;
			bool hideNotes = editHiSpeed.hideNotes;
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_FAST_FORWARD, getString("note_properties_hi_speed"), " ")
			            .c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				speedEdited |= UI::addFloatProperty(getString("hi_speed_speed"), speed, "%.3f");
				speedEdited |=
				    UI::addSelectProperty(getString("hi_speed_ease"), ease, hiSpeedEaseNames,
				                          arrayLength(hiSpeedEaseNames));

				speedEdited |=
				    UI::addFloatProperty(getString("hi_speed_skip_beat"), skip, 
					IO::formatString("%%.3f %s", getString("beat")).c_str());

				speedEdited |= UI::addCheckboxProperty(getString("hi_speed_hide_notes"), hideNotes);
				UI::endPropertyColumns();
			}

			if (speedEdited)
			{
				edited = true;
				for (auto& id : context.selectedHiSpeedChanges)
				{
					HiSpeedChange& hiSpeed = context.score.hiSpeedChanges.at(id);
					hiSpeed.speed = speed;
					hiSpeed.skips = skip;
					hiSpeed.ease = ease;
					hiSpeed.hideNotes = hideNotes;
				}
			}
		}

		if (edited)
			context.pushHistory("Edited object", prev, context.score);
	}
}
