#pragma once

#include "ScoreSerializer.h"
#include "../../BinaryReader.h"
#include "../../BinaryWriter.h"

namespace MikuMikuWorld
{
	enum class NativeScoreFormat
	{
		Auto,
		Monchi,
		UntitledCharts
	};

	class NativeScoreSerializer : public ScoreSerializer
	{
	private:
		struct ScoreVersion;

		NativeScoreFormat targetFormat;

		Note readNote(NoteType type, IO::BinaryReader& reader, const ScoreVersion& version,
		              unsigned int* extendedFlags = nullptr, EaseType* extendedEase = nullptr);
		ScoreMetadata readMetadata(IO::BinaryReader& reader, const ScoreVersion& version);
		void readScoreEvents(Score& score, IO::BinaryReader& reader, const ScoreVersion& version);
		void writeNote(const Note& note, IO::BinaryWriter& writer);
		void writeMetadata(const ScoreMetadata& metadata, IO::BinaryWriter& writer);
		void writeUntitledChartsMetadata(const ScoreMetadata& metadata, IO::BinaryWriter& writer);
		void writeScoreEvents(const Score& score, IO::BinaryWriter& writer);
		void serializeMonchi(const Score& score, std::string filename);
		void serializeUntitledCharts(const Score& score, std::string filename);

	  public:
		explicit NativeScoreSerializer(NativeScoreFormat targetFormat = NativeScoreFormat::Auto);
		void serialize(const Score& score, std::string filename) override;
		Score deserialize(std::string filename) override;

		static bool canSerialize(const Score& score);
		static bool hasMonchiNativeOnlyData(const Score& score);
	};
}
