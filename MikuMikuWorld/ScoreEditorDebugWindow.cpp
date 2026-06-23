#include "ScoreEditorWindows.h"
#include "ApplicationConfiguration.h"
#include "ScoreContext.h"
#include "ScoreEditorTimeline.h"
#include "UI.h"
#include "Utilities.h"

namespace MikuMikuWorld
{
	void DebugWindow::update(ScoreContext& context, ScoreEditorTimeline& timeline)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_BUG, "debug")))
		{
			constexpr ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
			constexpr ImGuiTreeNodeFlags treeNodeFlags = headerFlags | ImGuiTreeNodeFlags_Framed;
			if (ImGui::TreeNodeEx("Audio", treeNodeFlags))
			{
				if (ImGui::CollapsingHeader("Engine", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Sample Rate", context.audio.getDeviceSampleRate());
					UI::addReadOnlyProperty("Channel Count", context.audio.getDeviceChannelCount());
					UI::addReadOnlyProperty(
					    "Latency",
					    IO::formatString("%.2fms", context.audio.getDeviceLatency() * 1000));
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Music", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Music Initialized",
					                        boolToString(context.audio.isMusicInitialized()));
					UI::addReadOnlyProperty("Music Filename", context.audio.musicBuffer.name);

					float musicTime = context.audio.getMusicPosition(),
					      musicLength = context.audio.getMusicLength();
					int musicTimeSeconds = static_cast<int>(musicTime),
					    musicLengthSeconds = static_cast<int>(musicLength);
					UI::addReadOnlyProperty(
					    "Music Time",
					    IO::formatString(
					        "%02d:%02d/%02d:%02d", musicTimeSeconds,
					        static_cast<int>((musicTime - musicTimeSeconds) * 100),
					        musicLengthSeconds,
					        static_cast<int>((musicLength - musicLengthSeconds) * 100)));

					UI::addReadOnlyProperty("Sample Rate", context.audio.musicBuffer.sampleRate);
					UI::addReadOnlyProperty("Effective Sample Rate",
					                        context.audio.musicBuffer.effectiveSampleRate);
					UI::addReadOnlyProperty("Channel Count",
					                        context.audio.musicBuffer.channelCount);
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Waveform", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Waveform L",
					                        boolToString(!context.waveformL.isEmpty()));
					UI::addReadOnlyProperty("Waveform L Mip Count",
					                        context.waveformL.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform L Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::addReadOnlyProperty("Waveform R",
					                        boolToString(!context.waveformR.isEmpty()));
					UI::addReadOnlyProperty("Waveform R Mip Count",
					                        context.waveformR.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform R Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::endPropertyColumns();

					if (ImGui::Button("Re-Generate Waveform", { -1, UI::btnSmall.y }))
					{
						context.waveformL.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 0);
						context.waveformR.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 1);
					}
				}

				if (ImGui::CollapsingHeader("Sound Test", headerFlags))
				{
					constexpr ImGuiTableFlags tableFlags =
					    ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
					    ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
					    ImGuiTableFlags_RowBg;

					const int rowHeight = ImGui::GetFrameHeight() + 5;

					if (ImGui::BeginTable("##sound_test_table", 3, tableFlags, { -1, 200 }))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("Duration (sec)", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Play/Stop", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableHeadersRow();

						for (size_t i = 0;
						     i < arrayLength(SE_NAMES) * Audio::soundEffectsProfileCount; i++)
						{
							Audio::SoundInstance& sound = context.audio.debugSounds[i];

							ImGui::PushID(i);
							ImGui::TableNextRow(0, rowHeight);
							ImGui::TableSetColumnIndex(0);

							float ratio = sound.getCurrentFrame() /
							              static_cast<float>(sound.getLengthInFrames());
							if (!sound.isPlaying())
								ratio = 0.0f;

							ImGui::ProgressBar(ratio, { -1, 0 }, sound.name.c_str());

							float duration = sound.getDuration();
							int durationSecondsOnly = duration;
							float time = sound.isPlaying() ? sound.getCurrentTime() : 0.0f;
							int timeSecondsOnly = time;
							ImGui::TableSetColumnIndex(1);
							ImGui::Text("%02d:%02d/%02d:%02d", timeSecondsOnly,
							            static_cast<int>((time - timeSecondsOnly) * 100),
							            durationSecondsOnly,
							            static_cast<int>((duration - durationSecondsOnly) * 100));

							ImGui::TableSetColumnIndex(2);
							if (UI::transparentButton(ICON_FA_PLAY, UI::btnSmall))
							{
								sound.seek(0);
								sound.play();
							}

							ImGui::SameLine();
							ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
							ImGui::SameLine();
							if (UI::transparentButton(ICON_FA_STOP, UI::btnSmall))
								sound.stop();

							ImGui::PopID();
						}

						ImGui::EndTable();
					}
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Timeline", treeNodeFlags))
			{
				timeline.debug(context);
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}
}
