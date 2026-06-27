#include "../../Application.h"
#include "ScoreSerializeWindow.h"

#include "../../AudioTrackUtils.h"
#include "../../File.h"
#include "NativeScoreSerializer.h"
#include "SusSerializer.h"
#include "UscSerializer.h"
#include "SonolusSerializer.h"

#include "../../Localization.h"
#include "../../ApplicationConfiguration.h"
#include "../../Colors.h"
#include "../../ScoreEditor/ScoreEditor.h"
#include <algorithm>

namespace MikuMikuWorld
{
	static constexpr size_t FORMAT_COUNT = static_cast<size_t>(SerializeFormat::FormatCount);

	static const std::array<std::string, FORMAT_COUNT> FORMAT_NAMES = {
		IO::formatString("%s (%s)", IO::mchMmwsFilter.filterName.c_str(), MCH_MMWS_EXTENSION),
		IO::formatString("%s (%s)", IO::ucMmwsFilter.filterName.c_str(), UC_MMWS_EXTENSION),
		IO::formatString("%s (%s)", IO::mmwsFilter.filterName.c_str(), "*.mchmmws;*.unchmmws;*.ccmmws;*.mmws"),
		IO::formatString("%s (%s)", IO::susFilter.filterName.c_str(), SUS_EXTENSION),
		IO::formatString("%s (%s)", IO::uscFilter.filterName.c_str(), USC_EXTENSION),
		IO::lvlDatFilter.filterName,
	};

	static constexpr std::array<size_t, FORMAT_COUNT> EXPORT_AVAILABILITY = {
		false, false, false, true, false, true
	};

	static std::string formatSusIssue(const SusCompatibilityIssue& issue)
	{
		const char* text = getString(issue.textKey.c_str());
		return issue.count > 1 ? IO::formatString("%s (%d)", text, issue.count) : text;
	}

	static void drawSusIssueList(const std::vector<SusCompatibilityIssue>& issues)
	{
		for (const auto& issue : issues)
		{
			const std::string text = formatSusIssue(issue);
			ImGui::BulletText("%s", text.c_str());
		}
	}

	enum class SusCompatibilityPopupResult
	{
		None,
		Continue,
		Cancel,
	};

	static bool hasSusCompatibilityIssues(const SusCompatibilityReport& report)
	{
		return report.hasErrors() || report.hasWarnings();
	}

	static SusCompatibilityPopupResult drawSusCompatibilityPopup(
	    const SusCompatibilityReport& report)
	{
		SusCompatibilityPopupResult result = SusCompatibilityPopupResult::None;
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(),
		                        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSizeConstraints({ 480, 0 }, { 720, FLT_MAX });
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		const std::string title = IO::formatString("%s###sus_export_compatibility",
		                                           getString("sus_export_compatibility_title"));
		if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextWrapped("%s", getString("sus_export_compatibility_message"));
			ImGui::Spacing();

			if (report.hasErrors())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.30f, 1.0f),
				                   "%s", getString("sus_export_errors"));
				drawSusIssueList(report.errors);
				ImGui::Spacing();
			}

			if (report.hasWarnings())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.30f, 1.0f),
				                   "%s", getString("sus_export_warnings"));
				drawSusIssueList(report.warnings);
				ImGui::Spacing();
			}

			ImGui::Separator();
			const ImGuiStyle& style = ImGui::GetStyle();
			const float buttonWidth = 160.0f;
			const float spacing = style.ItemSpacing.x;
			const bool canContinue = !report.hasErrors();
			const float totalButtonWidth =
			    canContinue ? buttonWidth * 2.0f + spacing : buttonWidth;
			const float availableWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
			                     std::max(0.0f, availableWidth - totalButtonWidth));

			if (canContinue)
			{
				if (ImGui::Button(getString("sus_export_continue"), { buttonWidth, 0 }))
				{
					result = SusCompatibilityPopupResult::Continue;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}
			if (ImGui::Button(getString("cancel"), { buttonWidth, 0 }))
			{
				result = SusCompatibilityPopupResult::Cancel;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		return result;
	}

	DefaultScoreSerializeController::DefaultScoreSerializeController(Score score)
	{
		this->score = std::move(score);

		for (size_t i = 0; i < FORMAT_COUNT; i++)
			serializable.push_back(isSerializable(static_cast<SerializeFormat>(i), score));
	}

	DefaultScoreSerializeController::DefaultScoreSerializeController(Score score,
	                                                                 const std::string& filename)
	    : DefaultScoreSerializeController(std::move(score))
	{
		this->filename = filename;
		createSerializer();
	}

	bool DefaultScoreSerializeController::isSerializable(SerializeFormat format, const Score& score)
	{
		switch (format)
		{
		case SerializeFormat::MonchiNativeFormat:
			return NativeScoreSerializer::canSerialize(score);
		case SerializeFormat::UntitledChartsNativeFormat:
			return NativeScoreSerializer::canSerialize(score);
		case SerializeFormat::NativeFormat:
			return NativeScoreSerializer::canSerialize(score);
		case SerializeFormat::SusFormat:
			return true;
		case SerializeFormat::UscFormat:
			return UscSerializer::canSerialize(score);
		case SerializeFormat::LvlDataFormat:
			return PySekaiEngine::canSerialize(score);
		default:
			return false;
		}
	}

	void DefaultScoreSerializeController::createSerializer()
	{
		SerializeFormat format = toSerializeFormat(filename);
		switch (format)
		{
		case SerializeFormat::MonchiNativeFormat:
			serializer = std::make_unique<NativeScoreSerializer>(NativeScoreFormat::Monchi);
			break;
		case SerializeFormat::UntitledChartsNativeFormat:
			serializer =
			    std::make_unique<NativeScoreSerializer>(NativeScoreFormat::UntitledCharts);
			break;
		case SerializeFormat::NativeFormat:
			serializer = std::make_unique<NativeScoreSerializer>();
			break;
		case SerializeFormat::SusFormat:
			serializer = std::make_unique<SusSerializer>();
			break;
		case SerializeFormat::UscFormat:
			serializer = std::make_unique<UscSerializer>(config.minifyOutput);
			break;
		case SerializeFormat::LvlDataFormat:
			serializer = std::make_unique<SonolusSerializer>(
			    std::make_unique<PySekaiEngine>(), config.minifyOutput, !config.minifyOutput);
			break;
		default:
			errorMessage = "No serializer found!";
			serializer.reset();
			return;
		}

		if (format == SerializeFormat::MonchiNativeFormat ||
		    format == SerializeFormat::UntitledChartsNativeFormat ||
		    format == SerializeFormat::NativeFormat)
			scoreFilename = filename;
	}

	void DefaultScoreSerializeController::resetSusCompatibilityState()
	{
		susCompatibilityReport = {};
		susCompatibilityReportReady = false;
		susCompatibilityAccepted = false;
		pendingExportFileDialog = false;
	}

	bool DefaultScoreSerializeController::prepareSusCompatibilityReport()
	{
		if (selectedFormat != SerializeFormat::SusFormat)
			return true;
		if (!susCompatibilityReportReady)
		{
			susCompatibilityReport = SusSerializer::getCompatibilityReport(score);
			susCompatibilityReportReady = true;
			susCompatibilityAccepted = !hasSusCompatibilityIssues(susCompatibilityReport);
		}
		return susCompatibilityAccepted;
	}

	void DefaultScoreSerializeController::openExportFileDialog()
	{
		IO::FileDialog fileDialog{};
		fileDialog.title = "Export Chart";
		fileDialog.filters = { getFormatFilter(selectedFormat) };
		fileDialog.defaultExtension = getFormatDefaultExtension(selectedFormat);
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;

		if (fileDialog.saveFile() == IO::FileDialogResult::OK)
		{
			this->filename = fileDialog.outputFilename;
			createSerializer();
			ImGui::CloseCurrentPopup();
		}
	}

	SerializeResult DefaultScoreSerializeController::update()
	{
		if (serializer && !filename.empty())
		{
			SerializeFormat format = toSerializeFormat(filename);
			if (format == SerializeFormat::SusFormat && !susCompatibilityAccepted)
			{
				if (!susCompatibilityReportReady)
				{
					susCompatibilityReport = SusSerializer::getCompatibilityReport(score);
					susCompatibilityReportReady = true;
					if (!susCompatibilityReport.hasErrors() &&
					    !susCompatibilityReport.hasWarnings())
					{
						susCompatibilityAccepted = true;
					}
					else
					{
						ImGui::OpenPopup("###sus_export_compatibility");
					}
				}

				if (!susCompatibilityAccepted)
				{
					if (!ImGui::IsPopupOpen("###sus_export_compatibility"))
						ImGui::OpenPopup("###sus_export_compatibility");

					switch (drawSusCompatibilityPopup(susCompatibilityReport))
					{
					case SusCompatibilityPopupResult::Continue:
						susCompatibilityAccepted = true;
						break;
					case SusCompatibilityPopupResult::Cancel:
						return SerializeResult::Cancel;
					case SusCompatibilityPopupResult::None:
						break;
					}
					return SerializeResult::None;
				}
			}

			try
			{
				if (format == SerializeFormat::LvlDataFormat &&
				    AudioTrackUtils::hasAudioTrackEdits(score))
				{
					score.metadata.musicOffset =
					    AudioTrackUtils::getRenderedTimelineStartMs(score, score.metadata.musicOffset);
				}
				serializer->serialize(score, filename);
				if (format == SerializeFormat::LvlDataFormat &&
				    AudioTrackUtils::hasAudioTrackEdits(score))
				{
					std::string editedAudioFilename;
					Result audioResult = AudioTrackUtils::exportEditedAudio(
					    score, score.metadata.musicFile, IO::File::getFilepath(filename),
					    editedAudioFilename);
					if (!audioResult.isOk())
					{
						errorMessage = IO::formatString("%s\n%s: %s",
						                                "Failed to export edited audio",
						                                getString("error"),
						                                audioResult.getMessage().c_str());
						return SerializeResult::Error;
					}
				}
				serializer.reset();
				return SerializeResult::SerializeSuccess;
			}
			catch (const std::exception& err)
			{
				errorMessage = IO::formatString("%s\n"
				                                "%s: %s",
				                                getString("error_save_score_file"),
				                                getString("error"), err.what());
				return SerializeResult::Error;
			}
		}
		else if (filename.empty())
		{
			SerializeResult result = SerializeResult::None;
			if (!ImGui::IsPopupOpen("###serializer_picker"))
				ImGui::OpenPopup("###serializer_picker");

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
			                        ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize({ 560, 260 }, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			if (ImGui::BeginPopupModal(APP_NAME "###serializer_picker", NULL,
			                           ImGuiWindowFlags_NoResize))
			{
				const ImGuiStyle& style = ImGui::GetStyle();
				ImGui::Text(getString("export_as_file_format"));
				const bool canMinify = selectedFormat != SerializeFormat::SusFormat;
				bool minifyOutput = canMinify && config.minifyOutput;
				ImGui::BeginDisabled(!canMinify);
				if (ImGui::Checkbox(getString("minify"), &minifyOutput) && canMinify)
					config.minifyOutput = minifyOutput;
				ImGui::EndDisabled();
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float btnWidth = avail.x / 2 - style.ItemSpacing.x * 2,
				      btnHeight = ImGui::GetFrameHeight();

				if (ImGui::BeginListBox("###score_format_list",
				                        { avail.x, avail.y - btnHeight - style.ItemSpacing.y * 2 }))
				{
					for (int i = 0; i < FORMAT_NAMES.size(); ++i)
					{
						if (!EXPORT_AVAILABILITY[i])
							continue;
						bool isSelected = (static_cast<int>(selectedFormat) == i);
						if (isSelected)
						{
							const ImVec4 color = UI::accentColors[config.accentColor];
							const ImVec4 darkColor = generateDarkColor(color);
							const ImVec4 lightColor = generateHighlightColor(color);
							ImGui::PushStyleColor(ImGuiCol_Header, color);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
							ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
						}
						ImGui::BeginDisabled(!serializable[i]);
						if (ImGui::Selectable(FORMAT_NAMES[i].data(), isSelected))
						{
							if (selectedFormat != static_cast<SerializeFormat>(i))
								resetSusCompatibilityState();
							selectedFormat = static_cast<SerializeFormat>(i);
						}
						ImGui::EndDisabled();
						if (isSelected)
						{
							ImGui::PopStyleColor(3);
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndListBox();
				}

				ImGui::BeginDisabled(!isValidFormat(selectedFormat));
				if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
				{
					if (selectedFormat == SerializeFormat::SusFormat &&
					    !prepareSusCompatibilityReport())
					{
						pendingExportFileDialog = true;
						ImGui::OpenPopup("###sus_export_compatibility");
					}
					else
						pendingExportFileDialog = true;
				}
				ImGui::EndDisabled();
				ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
				if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
				{
					ImGui::CloseCurrentPopup();
					result = SerializeResult::Cancel;
				}

				switch (drawSusCompatibilityPopup(susCompatibilityReport))
				{
				case SusCompatibilityPopupResult::Continue:
					susCompatibilityAccepted = true;
					break;
				case SusCompatibilityPopupResult::Cancel:
					pendingExportFileDialog = false;
					break;
				case SusCompatibilityPopupResult::None:
					break;
				}

				if (pendingExportFileDialog &&
				    !ImGui::IsPopupOpen("###sus_export_compatibility"))
				{
					pendingExportFileDialog = false;
					openExportFileDialog();
				}
				ImGui::EndPopup();
			}
			return result;
		}
		else
			// Has filename but no valid serializer
			return SerializeResult::Error;
	}

	DefaultScoreDeserializeController::DefaultScoreDeserializeController(
	    const std::string& filename)
	{
		this->filename = filename;
		SerializeFormat format = toSerializeFormat(filename);
		if (isValidFormat(format))
		{
			selectedFormat = format;
			createDeserializer();
		}
	}

	void DefaultScoreDeserializeController::createDeserializer()
	{
		switch (selectedFormat)
		{
		case SerializeFormat::MonchiNativeFormat:
		case SerializeFormat::UntitledChartsNativeFormat:
		case SerializeFormat::NativeFormat:
			deserializer = std::make_unique<NativeScoreSerializer>();
			break;
		case SerializeFormat::SusFormat:
			deserializer = std::make_unique<SusSerializer>();
			break;
		case SerializeFormat::UscFormat:
			deserializer = std::make_unique<UscSerializer>();
			break;
		case SerializeFormat::LvlDataFormat:
			deserializer = std::make_unique<SonolusSerializer>(std::make_unique<PySekaiEngine>());
			break;
		default:
			deserializer.reset();
			filename.clear();
			errorMessage = "No deserializer found!";
			return;
		}

		if (selectedFormat == SerializeFormat::MonchiNativeFormat ||
		    selectedFormat == SerializeFormat::UntitledChartsNativeFormat ||
		    selectedFormat == SerializeFormat::NativeFormat)
			this->scoreFilename = filename;
	}

	SerializeResult DefaultScoreDeserializeController::update()
	{
		if (deserializer && !filename.empty())
		{
			try
			{
				score = deserializer->deserialize(filename);
				return SerializeResult::DeserializeSuccess;
			}
			catch (PartialScoreDeserializeError& partialError)
			{
				score = partialError.getScore();
				errorMessage = partialError.what();
				return SerializeResult::PartialDeserializeSuccess;
			}
			catch (std::exception& error)
			{
				errorMessage =
				    IO::formatString("%s\n"
				                     "%s: %s\n"
				                     "%s: %s",
				                     getString("error_load_score_file"), getString("score_file"),
				                     filename.c_str(), getString("error"), error.what());
				return SerializeResult::Error;
			}
		}
		else if (!filename.empty())
		{
			SerializeResult result = SerializeResult::None;
			if (!ImGui::IsPopupOpen("###deserializer_picker"))
				ImGui::OpenPopup("###deserializer_picker");

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
			                        ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize({ 560, 260 }, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			if (ImGui::BeginPopupModal(APP_NAME "###deserializer_picker", NULL,
			                           ImGuiWindowFlags_NoResize))
			{
				const ImGuiStyle& style = ImGui::GetStyle();
				ImGui::Text("%s '%s'\n%s", getString("unknown_file_format"), filename.c_str(),
				            getString("open_as_file_format"));
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float btnWidth = avail.x / 2 - style.ItemSpacing.x * 2,
				      btnHeight = ImGui::GetFrameHeight();

				if (ImGui::BeginListBox("###score_format_list",
				                        { avail.x, avail.y - btnHeight - style.ItemSpacing.y * 2 }))
				{
					for (int i = 0; i < FORMAT_NAMES.size(); ++i)
					{
						bool isSelected = (static_cast<int>(selectedFormat) == i);
						if (isSelected)
						{
							const ImVec4 color = UI::accentColors[config.accentColor];
							const ImVec4 darkColor = generateDarkColor(color);
							const ImVec4 lightColor = generateHighlightColor(color);
							ImGui::PushStyleColor(ImGuiCol_Header, color);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
							ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
						}
						if (ImGui::Selectable(FORMAT_NAMES[i].data(), isSelected))
							selectedFormat = static_cast<SerializeFormat>(i);
						if (isSelected)
						{
							ImGui::PopStyleColor(3);
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndListBox();
				}

				ImGui::BeginDisabled(!isValidFormat(selectedFormat));
				if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
				{
					createDeserializer();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndDisabled();
				ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
				if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
				{
					result = SerializeResult::Cancel;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			return result;
		}
		else
			return SerializeResult::Error;
	}

	void ScoreSerializeWindow::update(ScoreEditor& editor, ScoreContext& context,
	                                  ScoreEditorTimeline& timeline)
	{
		if (!controller)
			return;
		switch (controller->update())
		{
		case SerializeResult::PartialSerializeSuccess:
			IO::messageBox(APP_NAME, controller->getErrorMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning, Application::windowState.windowHandle);
			controller.reset();
			break;
		case SerializeResult::SerializeSuccess:
			IO::messageBox(APP_NAME, IO::formatString(getString("export_successful")),
			               IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Information,
			               Application::windowState.windowHandle);
			controller.reset();
			break;
		case SerializeResult::PartialDeserializeSuccess:
			IO::messageBox(APP_NAME, controller->getErrorMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Warning, Application::windowState.windowHandle);
			[[fallthrough]];
		case SerializeResult::DeserializeSuccess:
			context.clearSelection();
			context.history.clear();
			context.score = std::move(controller->getScore());
			context.selectedLayer = 0;
			context.workingData =
			    EditorScoreData(context.score.metadata, controller->getScoreFilename());

			editor.loadMusic(context.workingData.musicFilename);

			context.scoreStats.calculateStats(context.score);
			timeline.calculateMaxOffsetFromScore(context.score);

			UI::setWindowTitle((context.workingData.filename.size()
			                        ? IO::File::getFilename(context.workingData.filename)
			                        : windowUntitled));
			context.upToDate = true;
			if (!controller->getFilename().empty())
				editor.updateRecentFilesList(controller->getFilename());

			controller.reset();
			break;
		default:
		case SerializeResult::Error:
			IO::messageBox(APP_NAME, controller->getErrorMessage(), IO::MessageBoxButtons::Ok,
			               IO::MessageBoxIcon::Error, Application::windowState.windowHandle);
			controller.reset();
			break;
		case SerializeResult::Cancel:
			controller.reset();
			break;
		case SerializeResult::None:
			break;
		}
	}

	void ScoreSerializeWindow::serialize(const ScoreContext& context)
	{
		Score score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		controller = std::make_unique<DefaultScoreSerializeController>(std::move(score));
	}

	void ScoreSerializeWindow::serialize(const ScoreContext& context, const std::string& filename)
	{
		Score score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		controller = std::make_unique<DefaultScoreSerializeController>(std::move(score), filename);
	}

	void ScoreSerializeWindow::deserialize(const std::string& filename)
	{
		controller.reset();
		controller = std::make_unique<DefaultScoreDeserializeController>(filename);
	}
}
