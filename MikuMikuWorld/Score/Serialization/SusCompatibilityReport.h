#pragma once
#include <string>
#include <vector>

namespace MikuMikuWorld
{
	enum class SusCompatibilityIssueSeverity
	{
		Warning,
		Error,
	};

	struct SusCompatibilityIssue
	{
		SusCompatibilityIssueSeverity severity;
		std::string textKey;
		int count;
	};

	struct SusCompatibilityReport
	{
		std::vector<SusCompatibilityIssue> warnings;
		std::vector<SusCompatibilityIssue> errors;

		bool hasWarnings() const;
		bool hasErrors() const;
	};
}
