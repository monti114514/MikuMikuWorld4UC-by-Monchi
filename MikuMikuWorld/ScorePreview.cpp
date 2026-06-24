#include "Application.h"
#include "ScorePreview.h"
#include "PreviewEngine.h"
#include "Rendering/Camera.h"
#include "ResourceManager.h"
#include "IO.h"
#include "Colors.h"
#include "ImageCrop.h"
#include "ApplicationConfiguration.h"
#include "Score.h"
#include "Utilities.h"
#include <glad/glad.h>

namespace MikuMikuWorld
{
	struct PreviewPlaybackState
	{
		bool isPlaying{}, wasLastFramePlaying{};
	} playbackState;

	constexpr float EFFECTS_TARGET_ASPECT = 16.f / 9.f;

	namespace Utils
	{
		inline void fillRect(float target_width, float target_height, long double source_aspect_ratio, float& width, float& height)
		{
			const float target_aspect_ratio = target_width / target_height;
			width  = target_aspect_ratio < source_aspect_ratio ? (float)source_aspect_ratio * target_height : target_width;
			height = target_aspect_ratio > source_aspect_ratio ? target_width / (float)source_aspect_ratio : target_height;
		}
	};

	ScorePreviewWindow::ScorePreviewWindow() : previewBuffer{ 1920, 1080 }, background(), scaledAspectRatio(1)
	{
		noteEffectsCamera.setFov(50.f);
		noteEffectsCamera.setRotation(-90.f, 27.1f);
		noteEffectsCamera.setPosition({ 0, 5.32f, -5.86f, 0 });
		noteEffectsCamera.positionCamNormal();
	}

	ScorePreviewWindow::~ScorePreviewWindow() {}

	void ScorePreviewWindow::update(ScoreContext& context, Renderer* renderer)
	{
		bool isWindowActive =  !ImGui::IsWindowDocked() || ImGui::GetCurrentWindow()->TabId == ImGui::GetWindowDockNode()->SelectedTabId;
		if (!isWindowActive) return;

		bool needsRecalc = false;

		if (context.scorePreviewDrawData.noteSpeed != config.pvNoteSpeed)
			needsRecalc = true;

		if (!needsRecalc
			&& context.scorePreviewDrawData.layerForceNoteSpeeds.size() != context.score.layers.size())
			needsRecalc = true;

		if (!needsRecalc)
		{
			for (int layer = 0; layer < static_cast<int>(context.score.layers.size()); ++layer)
			{
				if (context.scorePreviewDrawData.layerForceNoteSpeeds[layer] != Engine::getLayerForceNoteSpeed(context.score, layer))
				{
					needsRecalc = true;
					break;
				}
			}
		}

		if (!needsRecalc && !context.score.notes.empty() && context.scorePreviewDrawData.drawingNotes.empty())
			needsRecalc = true;

		if (!needsRecalc && !context.scorePreviewDrawData.drawingNotes.empty())
		{
			int checkCount = 0;
			for (const auto& note : context.scorePreviewDrawData.drawingNotes)
			{
				if (context.score.notes.find(note.refID) == context.score.notes.end())
				{
					needsRecalc = true;
					break;
				}
				if (++checkCount > 5) break; 
			}
		}

		if (needsRecalc)
		{
			context.scorePreviewDrawData.calculateDrawData(context.score);
		}

		ImVec2 size = ImGui::GetContentRegionAvail() - ImVec2{ this->getScrollbarWidth(), 0 };
		ImVec2 position = ImGui::GetCursorScreenPos();
		ImRect boundaries = ImRect(position, position + size);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);
		
		if (config.drawBackground && background.shouldUpdate(context.workingData.jacket))
			background.update(renderer, context.workingData.jacket);

		if (!context.scorePreviewDrawData.effectView.isInitialized())
			context.scorePreviewDrawData.effectView.init();

		if (playbackState.isPlaying)
			context.scorePreviewDrawData.effectView.update(context);
		else if (playbackState.wasLastFramePlaying)
			context.scorePreviewDrawData.effectView.reset();

		static int shaderId = ResourceManager::getShader("basic2d");
		static int pteShaderId = ResourceManager::getShader("particles");
		if (shaderId == -1 || pteShaderId == -1) return;

		Shader* shader = ResourceManager::shaders[shaderId];
		Shader* pteShader = ResourceManager::shaders[pteShaderId];
		shader->use();

		float width  = size.x, height = size.y;
		float scaledWidth = Engine::STAGE_TARGET_WIDTH * Engine::STAGE_WIDTH_RATIO;
		float scaledHeight = Engine::STAGE_TARGET_HEIGHT * Engine::STAGE_HEIGHT_RATIO;
		float scrTop  = Engine::STAGE_TARGET_HEIGHT * Engine::STAGE_TOP_RATIO;
		Utils::fillRect(Engine::STAGE_TARGET_WIDTH, Engine::STAGE_TARGET_HEIGHT, size.x / size.y, width, height);
		
		float aspectRatio = width / height;
		scaledAspectRatio = scaledWidth / scaledHeight;

		auto view = DirectX::XMMatrixScaling(scaledWidth, scaledHeight, 1.f) * DirectX::XMMatrixTranslation(0.f, -scrTop, 0.f);
		auto projection = Camera().getOffCenterOrthographicProjection(-width / 2, width / 2, height / 2, -height / 2);
		auto viewProjection = DirectX::XMMatrixMultiply(view, projection); 
		const auto pView = noteEffectsCamera.getViewMatrix();
		auto pProjection = noteEffectsCamera.getProjectionMatrix(aspectRatio, 0.3f, 1000.f);
		float projectionScale = std::min(aspectRatio / EFFECTS_TARGET_ASPECT, 1.f);
		pProjection = DirectX::XMMatrixScaling(projectionScale, -projectionScale, 1.f) * pProjection;

		shader->setMatrix4("projection", viewProjection);
		float currentTime = context.getTimeAtCurrentTick();

		if (previewBuffer.getWidth() != (unsigned int)size.x || previewBuffer.getHeight() != (unsigned int)size.y)
			previewBuffer.resize((unsigned int)size.x, (unsigned int)size.y);
		previewBuffer.bind();
		previewBuffer.clear();

		renderer->beginBatch();
		if (config.drawBackground)
		{
			background.setBrightness(config.pvBackgroundBrightness);
			background.draw(renderer, width, height);
		}
		drawStage(renderer);
		renderer->endBatch();

		context.scorePreviewDrawData.effectView.updateEffects(context, noteEffectsCamera, currentTime);

		shader->use();
		shader->setMatrix4("projection", viewProjection);
		renderer->beginBatch();
		drawLines(context, renderer);
		drawHoldCurves(context, renderer);
		if (config.pvStageCover != 0) {
			drawStageCoverMask(renderer);
			renderer->endBatchWithDepthTest(GL_LEQUAL);
		}
		else
			renderer->endBatch();

		pteShader->use();
		pteShader->setMatrix4("projection", pProjection);
		pteShader->setMatrix4("view", pView);
		renderer->beginBatch();
		context.scorePreviewDrawData.effectView.drawUnderNoteEffects(renderer, currentTime);
		renderer->endBatchWithBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		shader->use();
		shader->setMatrix4("projection", viewProjection);
		renderer->beginBatch();

		drawHoldTicks(context, renderer);
		drawNotes(context, renderer);
		if (config.pvStageCover != 0) {
			drawStageCoverMask(renderer);
			drawStageCover(renderer);
			drawStageCoverDecoration(renderer);
			renderer->endBatchWithDepthTest(GL_LEQUAL);
		}
		else
			renderer->endBatch();

		pteShader->use();
		pteShader->setMatrix4("projection", pProjection);
		pteShader->setMatrix4("view", pView);
		renderer->beginBatch();
		context.scorePreviewDrawData.effectView.drawEffects(renderer, currentTime);
		renderer->endBatchWithBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		drawList->AddImage((ImTextureID)(size_t)previewBuffer.getTexture(), position, position + size, {0, 0}, {1, 1});
	}

	void ScorePreviewWindow::updateUI(ScoreEditorTimeline& timeline, ScoreContext& context)
	{
		updateToolbar(timeline, context);
		ImGuiIO io = ImGui::GetIO();
		float mouseWheel = io.MouseWheel * 1;
		if (!timeline.isPlaying() && ImGui::IsWindowHovered() && mouseWheel != 0)
			context.currentTick += std::max(mouseWheel * TICKS_PER_BEAT / 2, (float)-context.currentTick);
		updateScrollbar(timeline, context);

		playbackState.wasLastFramePlaying = playbackState.isPlaying;
		playbackState.isPlaying = timeline.isPlaying();

		if (isFullWindow())
		{
			if (ImGui::BeginPopupContextWindow("preview_context_menu", 1 ))
			{
				bool _fullWindow = fullWindow;
				if (ImGui::MenuItem(getString("fullscreen_preview"), NULL, &_fullWindow))
					setFullWindow(_fullWindow);

				ImGui::MenuItem(getString("preview_draw_toolbar"), NULL, &config.pvDrawToolbar);
				ImGui::MenuItem(getString("return_to_last_tick"), NULL, &config.returnToLastSelectedTickOnPause);
				ImGui::EndPopup();
			}
		}
	}

	void ScorePreviewWindow::setFullWindow(bool _fullWindow)
	{
		fullWindow = _fullWindow;
	}

}
