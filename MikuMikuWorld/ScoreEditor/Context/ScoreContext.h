#pragma once
#include "../../Audio/AudioManager.h"
#include "../../Audio/Waveform.h"
#include "../../Constants.h"
#include "HistoryManager.h"
#include "../../Jacket.h"
#include "../../JsonIO.h"
#include "../../Score.h"
#include "../../ScoreStats.h"
#include "TimelineMode.h"
#include "PreviewData.h"
#include <cstdint>
#include <set>
#include <unordered_set>

namespace MikuMikuWorld
{
	enum class TimelineEditTarget : uint8_t
	{
		Notes,
		Audio,
	};

	struct EditArgs
	{
		int noteWidth{ 3 };
		FlickType flickType{ FlickType::Default };
		HoldStepType stepType{ HoldStepType::Normal };
		EaseType easeType{ EaseType::Linear };
		GuideColor colorType{ GuideColor::Green };
		FadeType fadeType{ FadeType::Out };
		HoldEndType holdStartType{ HoldEndType::Normal };
		HoldEndType holdEndType{ HoldEndType::Normal };

		float bpm{ 160.0f };
		int timeSignatureNumerator{ 4 };
		int timeSignatureDenominator{ 4 };
		float hiSpeed{ 1.0f };
		float hiSpeedSkip{ 0.0f };
		HiSpeedEaseType hiSpeedEase{ HiSpeedEaseType::None };
		bool hiSpeedHideNotes{ false };
	};

	class EditorScoreData
	{
	  public:
		std::string title{};
		std::string designer{};
		std::string artist{};
		std::string filename{};
		std::string musicFilename{};
		float musicOffset{};
		int laneExtension{};
		int baseLifePoint{ 1000 };
		Jacket jacket{};

		EditorScoreData() {}
		EditorScoreData(const ScoreMetadata& metadata, const std::string& filename)
		    : title{ metadata.title }, designer{ metadata.author }, artist{ metadata.artist },
		      musicFilename{ metadata.musicFile }, musicOffset{ metadata.musicOffset },
		      laneExtension{ metadata.laneExtension }, baseLifePoint{ metadata.baseLifePoint }
		{
			this->filename = filename;
			jacket.load(metadata.jacketFile);
		}

		ScoreMetadata toScoreMetadata() const
		{
			return { title, artist, designer, musicFilename, jacket.getFilename(), musicOffset,
				     laneExtension, baseLifePoint };
		}
	};

	struct PasteData
	{
		std::unordered_map<id_t, Note> notes;
		std::unordered_map<id_t, HoldNote> holds;
		std::unordered_map<id_t, Note> damages;
		std::unordered_map<id_t, HiSpeedChange> hiSpeedChanges;
		std::vector<Waypoint> waypoints;
		std::vector<Tempo> tempoChanges;
		std::vector<TimeSignature> timeSignatures;
		std::vector<SkillTrigger> skills;
		bool hasFever{ false };
		Fever fever{ -1, -1 };
		bool pasting{ false };
		int offsetTicks{};
		int offsetLane{};
		int midLane{};
		int minLaneOffset{};
		int maxLaneOffset{};
	};

	enum class MetaEventKind : uint8_t
	{
		Waypoint,
		Bpm,
		TimeSignature,
		Skill,
		FeverStart,
		FeverEnd
	};

	struct SelectedMetaEvent
	{
		MetaEventKind kind = MetaEventKind::Waypoint;
		id_t key = static_cast<id_t>(-1);

		bool operator<(const SelectedMetaEvent& other) const
		{
			if (kind != other.kind)
				return static_cast<uint8_t>(kind) < static_cast<uint8_t>(other.kind);
			return key < other.key;
		}

		bool operator==(const SelectedMetaEvent& other) const
		{
			return kind == other.kind && key == other.key;
		}
	};

	class ScoreContext
	{
	  public:
		Score score;
		EditorScoreData workingData;
		ScoreStats scoreStats;
		HistoryManager history;
		Audio::AudioManager audio;
		PasteData pasteData{};
		Engine::DrawData scorePreviewDrawData;
		
		std::unordered_set<id_t> selectedNotes;
		std::unordered_set<id_t> selectedHiSpeedChanges;
		std::set<SelectedMetaEvent> selectedMetaEvents;

		Audio::WaveformMipChain waveformL, waveformR;
		Audio::WaveformMipChain sourceWaveformL, sourceWaveformR;

		int currentTick{};
		bool upToDate{ true };

		int selectedLayer = 0;
		bool showAllLayers = false;
		TimelineEditTarget timelineEditTarget = TimelineEditTarget::Notes;
		id_t selectedAudioClip = static_cast<id_t>(-1);

		bool isAudioEditing() const { return timelineEditTarget == TimelineEditTarget::Audio; }
		bool isNotesEditing() const { return timelineEditTarget == TimelineEditTarget::Notes; }

		bool hasSelection() const
		{
			return selectedNotes.size() > 0 || selectedHiSpeedChanges.size() > 0 ||
			       selectedMetaEvents.size() > 0 || selectedAudioClip != static_cast<id_t>(-1);
		}

		bool hasHoldInSelection() const
		{
			for (id_t id : selectedNotes)
			{
				const Note& n = score.notes.at(id);
				if (n.getType() == NoteType::Hold || n.getType() == NoteType::HoldMid ||
				    n.getType() == NoteType::HoldEnd)
					return true;
			}
			return false;
		}

		std::unordered_set<int> getHoldsFromSelection()
		{
			std::unordered_set<int> holds;
			for (id_t id : selectedNotes)
			{
				const Note& note = score.notes.at(id);
				if (note.getType() == NoteType::Hold)
					holds.insert(note.ID);
				else if (note.getType() == NoteType::HoldMid || note.getType() == NoteType::HoldEnd)
					holds.insert(note.parentID);
			}

			return holds;
		}

		double getTimeAtCurrentTick() const
		{
			return accumulateDuration(currentTick, TICKS_PER_BEAT, score.tempoChanges);
		}

		bool selectionHasEase() const;
		bool selectionHasHold() const;
		bool selectionHasStep() const;
		bool selectionHasFlickable() const;
		bool selectionCanConnect() const;
		bool selectionCanChangeHoldType() const;
		bool selectionCanChangeFadeType() const;
		inline bool isNoteSelected(const Note& note)
		{
			return selectedNotes.find(note.ID) != selectedNotes.end();
		}
		inline void selectAll()
		{
			selectedNotes.clear();
			for (auto& it : score.notes)
				selectedNotes.insert(it.first);
		}
		inline void clearSelection()
		{
			selectedNotes.clear();
			selectedHiSpeedChanges.clear();
			selectedMetaEvents.clear();
			selectedAudioClip = static_cast<id_t>(-1);
		}
		bool isMetaEventSelected(const SelectedMetaEvent& event) const;
		void selectMetaEvent(const SelectedMetaEvent& event, bool additive);
		void toggleMetaEventSelection(const SelectedMetaEvent& event);
		int getMetaEventTick(const SelectedMetaEvent& event) const;
		void assignMissingMetaEventRuntimeIds();

		void setStep(HoldStepType step);
		void setFlick(FlickType flick);
		void setEase(EaseType ease);
		void setHoldType(HoldNoteType hold);
		void setFadeType(FadeType fade);
		void setGuideColor(GuideColor color);
		void setLayer(int layer);
		void toggleCriticals();
		void toggleFriction();
		void toggleDummy();

		void deleteSelection();
		void flipSelection();
		void cutSelection();
		void copySelection();
		void paste(bool flip);
		void duplicateSelection(bool flip);
		void doPasteData(const nlohmann::json& data, bool flip);
		void cancelPaste();
		void confirmPaste();
		void shrinkSelection(Direction direction);
		void compressSelection();

		void connectHoldsInSelection();
		void splitHoldInSelection();
		void repeatMidsInSelection();
		/**
		 * @brief Convert normal holds or guide notes within selection into traces
		 * @param division Current division. Used to determine the ticks between two trace notes
		 * @param deleteOrigin Delete the original hold notes or not
		 */
		void convertHoldToTraces(int division, bool deleteOrigin);

		void lerpHiSpeeds(int division, EaseType ease);

		void convertHoldToGuide(GuideColor color);
		void convertGuideToHold();

		void undo();
		void redo();
		void pushHistory(std::string description, const Score& prev, const Score& current);
	};
}
