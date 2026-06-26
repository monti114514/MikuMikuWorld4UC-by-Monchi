#pragma once

#include "../../ImGui/imgui_internal.h"
#include "../../Math.h"

namespace MikuMikuWorld
{
	namespace TimelineEventControls
	{
		bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt, bool enabled,
		                  bool selected = false, ImRect* outRect = nullptr);

		bool metaClusterControl(const char* id, float lineStartX, Vector2 pos, float width,
		                        ImU32 color, const char* txt, bool enabled,
		                        bool selected = false, ImRect* outRect = nullptr);
	}
}
