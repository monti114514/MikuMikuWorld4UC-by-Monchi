#include "ScorePreview.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "ImageCrop.h"
#include "PreviewEngine.h"
#include "ResourceManager.h"
#include "Utilities.h"

namespace MikuMikuWorld
{
	namespace
	{
		Color stageDefaultTint{ 1.f, 1.f, 1.f, 1.f };

		DirectX::XMFLOAT4 stageToFloat4(const Color& color, float alphaScale = 1.0f)
		{
			return { color.r, color.g, color.b, color.a * alphaScale };
		}

		std::array<DirectX::XMFLOAT4, 4> stageGetUV(float left, float right, float top, float bottom)
		{
			return {{
				{ right, top, 0.f, 1.f },
				{ right, bottom, 0.f, 1.f },
				{ left, bottom, 0.f, 1.f },
				{ left, top, 0.f, 1.f },
			}};
		}
	}

	void ScorePreviewWindow::drawStage(Renderer* renderer)
	{
		int index = ResourceManager::getTexture("stage");
		if (index == -1) return;
		const Texture& stage = ResourceManager::textures[index];
		if (!isArrayIndexInBounds(SPR_SEKAI_STAGE, stage.sprites)) return;
		const Sprite& stageSprite = stage.sprites[SPR_SEKAI_STAGE];
		constexpr float stageWidth = (Engine::STAGE_TEX_WIDTH / Engine::STAGE_LANE_WIDTH) * Engine::STAGE_NUM_LANES;
		constexpr float stageLeft = -stageWidth / 2;
		constexpr float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		constexpr float stageHeight = Engine::STAGE_TEX_HEIGHT / Engine::STAGE_LANE_HEIGHT;

		renderer->drawRectangle(Vector2(stageLeft, stageTop), Vector2(stageWidth, stageHeight),
		                        stage,
		                        stageSprite.getX(), stageSprite.getX() + stageSprite.getWidth(),
		                        stageSprite.getY(), stageSprite.getY() + stageSprite.getHeight(),
		                        Color(stageDefaultTint.r, stageDefaultTint.g, stageDefaultTint.b,
		                              stageDefaultTint.a * config.pvStageOpacity),
		                        -1);
	}

	void ScorePreviewWindow::drawStageCoverMask(Renderer* renderer)
	{
		int index = ResourceManager::getTexture("stage");
		if (index == -1) return;
		const Texture& stage = ResourceManager::textures[index];
		constexpr float stageWidth = (Engine::STAGE_TEX_WIDTH / Engine::STAGE_LANE_WIDTH) * Engine::STAGE_NUM_LANES;
		constexpr float stageLeft = -stageWidth / 2, stageRight = stageWidth / 2;

		constexpr float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float stageHeight = config.pvStageCover * (1 - stageTop);

		static auto model = DirectX::XMMatrixTranslation(0, 0, 1);
		auto vPos = Engine::quadvPos(stageLeft, stageRight, stageTop + stageHeight, 0);
		auto uv = stageGetUV(0.f, 1.f, 0.f, 1.f);
		renderer->pushQuad(vPos, uv, model, stageToFloat4(stageDefaultTint, 0.f), (int)stage.getID(), 0);
	}

	void ScorePreviewWindow::drawStageCover(Renderer* renderer)
	{
		int index = ResourceManager::getTexture("stage");
		if (index == -1) return;
		const Texture& stage = ResourceManager::textures[index];
		if (!isArrayIndexInBounds(SPR_SEKAI_STAGE, stage.sprites)) return;
		const Sprite& stageSprite = stage.sprites[SPR_SEKAI_STAGE];
		constexpr float stageWidth = (Engine::STAGE_TEX_WIDTH / Engine::STAGE_LANE_WIDTH) * Engine::STAGE_NUM_LANES;
		const float stageLeft = -stageWidth / 2, stageRight = stageWidth / 2;

		constexpr float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float stageHeight = config.pvStageCover * (1 - stageTop);
		const float spriteHeight = config.pvStageCover * (Engine::STAGE_LANE_HEIGHT - Engine::STAGE_LANE_TOP);

		static auto model = DirectX::XMMatrixTranslation(0, 0, 1);

		float texW = (float)stage.getWidth();
		float texH = (float)stage.getHeight();
		auto vPos = Engine::quadvPos(stageLeft, stageRight, stageTop + stageHeight, stageTop);
		auto uv = stageGetUV(stageSprite.getX() / texW,
		                     (stageSprite.getX() + stageSprite.getWidth()) / texW,
		                     stageSprite.getY() / texH,
		                     (stageSprite.getY() + spriteHeight) / texH);

		renderer->pushQuad(vPos, uv, model,
		                   DirectX::XMFLOAT4(0, 0, 0, config.pvStageOpacity),
		                   (int)stage.getID(), 0);
	}

	void ScorePreviewWindow::drawStageCoverDecoration(Renderer* renderer)
	{
		if (noteTextures.notes == -1)
			return;

		constexpr float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const Texture& noteTex = getNoteTexture();
		size_t sprIndex = SPR_SIMULTANEOUS_CONNECTION;
		size_t transIndex = static_cast<size_t>(SpriteType::SimultaneousLine);
		if (!isArrayIndexInBounds(sprIndex, noteTex.sprites)) return;
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms)) return;

		const SpriteTransform& lineTransform = ResourceManager::spriteTransforms[transIndex];
		const Sprite& sprite = noteTex.sprites[sprIndex];
		float x = 0.12f * (1.f - config.pvStageCover);
		auto vPos = lineTransform.apply(Engine::perspectiveQuadvPos(
		    -6.f - x, 6.f + x,
		    1.f + Engine::getNoteHeight(), 1.f - Engine::getNoteHeight()));
		float y = stageTop + config.pvStageCover * (1.f - stageTop);
		auto uv = Engine::quadUV(sprite, noteTex);
		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		renderer->pushQuad(vPos, uv, model,
		                   stageToFloat4(stageDefaultTint, config.pvStageOpacity),
		                   (int)noteTex.getID(), -1);
	}
}
