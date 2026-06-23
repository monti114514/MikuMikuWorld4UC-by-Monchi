#include "ScoreContext.h"

#include "File.h"
#include "IO.h"
#include "UI.h"

#include <algorithm>

namespace MikuMikuWorld
{
	void ScoreContext::undo()
	{
		if (history.hasUndo())
		{
			score = history.undo();
			assignMissingMetaEventRuntimeIds();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size()
			                        ? IO::File::getFilename(workingData.filename)
			                        : windowUntitled) +
			                   "*");
			upToDate = false;

			scoreStats.calculateStats(score);
			scorePreviewDrawData.calculateDrawData(score);
		}
	}

	void ScoreContext::redo()
	{
		if (history.hasRedo())
		{
			score = history.redo();
			assignMissingMetaEventRuntimeIds();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size()
			                        ? IO::File::getFilename(workingData.filename)
			                        : windowUntitled) +
			                   "*");
			upToDate = false;

			scoreStats.calculateStats(score);
			scorePreviewDrawData.calculateDrawData(score);
		}
	}

	void ScoreContext::pushHistory(std::string description, const Score& prev, const Score& curr)
	{
		history.pushHistory(description, prev, curr);

		UI::setWindowTitle((workingData.filename.size() ? IO::File::getFilename(workingData.filename)
		                                                : windowUntitled) +
		                   "*");
		scoreStats.calculateStats(score);
		scorePreviewDrawData.calculateDrawData(score);

		upToDate = false;
	}

	bool ScoreContext::selectionHasEase() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
		                   [this](const int id) { return score.notes.at(id).hasEase(); });
	}

	bool ScoreContext::selectionHasHold() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
		                   [this](int id)
		                   { return score.notes.at(id).getType() == NoteType::Hold; });
	}

	bool ScoreContext::selectionHasStep() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(), [this](const int id)
		                   { return score.notes.at(id).getType() == NoteType::HoldMid; });
	}

	bool ScoreContext::selectionHasFlickable() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
		                   [this](const int id) { return score.notes.at(id).canFlick(); });
	}

	bool ScoreContext::selectionCanConnect() const
	{
		if (selectedNotes.size() != 2)
			return false;

		const auto& note1 = score.notes.at(*selectedNotes.begin());
		const auto& note2 = score.notes.at(*std::next(selectedNotes.begin()));
		if (note1.tick == note2.tick)
			return (note1.getType() == NoteType::Hold && note2.getType() == NoteType::HoldEnd) ||
			       (note1.getType() == NoteType::HoldEnd && note2.getType() == NoteType::Hold);

		auto noteTickCompareFunc = [](const Note& n1, const Note& n2) { return n1.tick < n2.tick; };
		Note earlierNote = std::min(note1, note2, noteTickCompareFunc);
		Note laterNote = std::max(note1, note2, noteTickCompareFunc);

		return (earlierNote.getType() == NoteType::HoldEnd &&
		        laterNote.getType() == NoteType::Hold);
	}

	bool ScoreContext::selectionCanChangeHoldType() const
	{
		return std::any_of(
		    selectedNotes.begin(), selectedNotes.end(),
		    [this](const int id)
		    {
			    const Note& note = score.notes.at(id);
			    if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
				    return !score.holdNotes
				                .at(note.getType() == NoteType::Hold ? note.ID : note.parentID)
				                .isGuide();

			    return false;
		    });
	}

	bool ScoreContext::selectionCanChangeFadeType() const
	{
		return std::any_of(
		    selectedNotes.begin(), selectedNotes.end(),
		    [this](const int id)
		    {
			    const Note& note = score.notes.at(id);
			    if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
				    return score.holdNotes
				        .at(note.getType() == NoteType::Hold ? note.ID : note.parentID)
				        .isGuide();

			    return false;
		    });
	}
}
