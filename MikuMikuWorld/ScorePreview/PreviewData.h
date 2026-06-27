#pragma once
#include <random>
#include "../Score.h"
#include "../MathUtils.h"
#include "EffectView.h"

namespace MikuMikuWorld
{
	struct Score;
	struct Note;
	struct ScoreContext;
}

namespace MikuMikuWorld::Engine
{
	struct DrawingNote
	{
		int refID;
		Range visualTime;
		NoteType type;
		bool dummy;
		int layer;
	};

	struct DrawingLine
	{
		int leftTick;
		float leftLane;
		int leftLayer;

		int rightTick;
		float rightLane;
		int rightLayer;
	};

	struct DrawingHoldTick
	{
		int refID;
		float center;
		Range visualTime;
		bool dummy;
		int layer;
	};

	struct DrawingHoldSegment
	{
		int endID;
		EaseType ease;
		bool isGuide;
		GuideColor color;
		bool dummy;
		int layer;
		HoldStepLayer stepLayer{ HoldStepLayer::Top };

		ptrdiff_t tailStepIndex;
		double headTime, tailTime;
		float headLeft, headRight;
		float tailLeft, tailRight;
		float startTime, endTime;
		double activeTime;

		int startTick;
		int endTick;
	};

	struct HiSpeedCacheNode
	{
		int tick;
		double time;
		double stm;
		double secondsPerTick;
		double speed;
		double speedSlope;
	};

	struct LayerHiSpeedCache
	{
		std::vector<HiSpeedCacheNode> nodes;
		double getStm(int tick) const;
	};

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;

		std::vector<LayerHiSpeedCache> hsCache;
		std::vector<float> layerForceNoteSpeeds;

		Effect::EffectView effectView;

		void clear();
		void calculateDrawData(Score const& score);
	};
}
