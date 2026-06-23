#include "ScoreEditorTimeline.h"

#include "Constants.h"

#include <algorithm>
#include <cmath>

namespace MikuMikuWorld
{
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
}
