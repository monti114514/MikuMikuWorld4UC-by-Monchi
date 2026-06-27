#pragma once
#include "ScoreSerializer.h"
#include "SusCompatibilityReport.h"
#include <string>

namespace MikuMikuWorld
{
	struct SUS;

	class SusSerializer : public ScoreSerializer
	{
	  public:
		static Score susToScore(const SUS& sus);
		static SUS scoreToSus(const Score& score);
		static SusCompatibilityReport getCompatibilityReport(const Score& score);

		void serialize(const Score& score, std::string filename) override;
		Score deserialize(std::string filename) override;

		static bool canSerialize(const Score& score);
	};
}
