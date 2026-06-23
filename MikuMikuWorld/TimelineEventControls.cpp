#include "TimelineEventControls.h"
#include "UI.h"

#include <algorithm>
#include <cmath>

namespace MikuMikuWorld
{
	namespace TimelineEventControls
	{
		bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt, bool enabled,
		                  bool selected, ImRect* outRect)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			if (!drawList)
				return false;

			pos.x = floorf(pos.x);
			pos.y = floorf(pos.y);

			ImGui::PushID(pos.y);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 0 });
			bool activated = UI::coloredButton(
			    txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { -1, -1 }, color,
			    enabled);
			ImGui::PopStyleVar();
			ImGui::PopID();

			ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			if (outRect != nullptr)
				*outRect = rect;

			drawList->AddLine({ xPos, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y },
			                  color, primaryLineThickness);
			if (selected)
				drawList->AddRect(rect.Min, rect.Max, 0xffffffff, 2.0f,
				                  ImDrawFlags_RoundCornersAll, 1.5f);

			return activated;
		}

		bool metaClusterControl(const char* id, float lineStartX, Vector2 pos, float width,
		                        ImU32 color, const char* txt, bool enabled, bool selected,
		                        ImRect* outRect)
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

			drawList->AddLine({ lineStartX, pos.y },
			                  { pos.x + ImGui::GetItemRectSize().x, pos.y }, color,
			                  primaryLineThickness);
			if (selected)
				drawList->AddRect(rect.Min, rect.Max, 0xffffffff, 2.0f,
				                  ImDrawFlags_RoundCornersAll, 1.5f);

			return activated;
		}
	}
}
