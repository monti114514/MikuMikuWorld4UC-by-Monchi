#include "ScorePreview.h"
#include "../Application.h"
#include "../ApplicationConfiguration.h"
#include "../Colors.h"
#include "ImageCrop.h"
#include "../IO.h"
#include "PreviewEngine.h"
#include "../Rendering/Camera.h"
#include "../ResourceManager.h"
#include <glad/glad.h>

namespace MikuMikuWorld
{
	namespace
	{
		Color backgroundDefaultTint{ 1.f, 1.f, 1.f, 1.f };

		void backgroundFillRect(float targetWidth, float targetHeight, long double sourceAspectRatio,
		                        float& width, float& height)
		{
			const float targetAspectRatio = targetWidth / targetHeight;
			width = targetAspectRatio < sourceAspectRatio
			            ? static_cast<float>(sourceAspectRatio) * targetHeight
			            : targetWidth;
			height = targetAspectRatio > sourceAspectRatio
			             ? targetWidth / static_cast<float>(sourceAspectRatio)
			             : targetHeight;
		}

		std::array<DirectX::XMFLOAT4, 4> backgroundGetUV(float left, float right, float top, float bottom)
		{
			return {{
			    { right, top, 0.f, 1.f },
			    { right, bottom, 0.f, 1.f },
			    { left, bottom, 0.f, 1.f },
			    { left, top, 0.f, 1.f },
			}};
		}
	}

	ScorePreviewBackground::ScorePreviewBackground() : backgroundFile(), jacketFile{}, brightness(0.5f), frameBuffer{2048, 2048}, init{false} {}

	ScorePreviewBackground::~ScorePreviewBackground()
	{
		frameBuffer.dispose();
	}

	void ScorePreviewBackground::setBrightness(float value)
	{
		brightness = value;
	}

	void ScorePreviewBackground::update(Renderer* renderer, const Jacket& jacket)
	{
		init = true;
		backgroundFile = config.backgroundImage;
		jacketFile = jacket.getFilename();
		brightness = config.pvBackgroundBrightness;
		bool useDefaultTexture = backgroundFile.empty() || !IO::File::exists(backgroundFile);
		
		Texture backgroundTex = { useDefaultTexture ? Application::getAppDir() + "res\\textures\\default.png" : backgroundFile};
		const float bgWidth = backgroundTex.getWidth(), bgHeight = backgroundTex.getHeight();
		if (bgWidth != frameBuffer.getWidth() || bgHeight != frameBuffer.getHeight())
			frameBuffer.resize((unsigned int)bgWidth, (unsigned int)bgHeight);
		frameBuffer.bind();
		frameBuffer.clear();
		
		int shaderId;
		if ((shaderId = ResourceManager::getShader("basic2d")) == -1) return;
		Shader* basicShader = ResourceManager::shaders[shaderId];
		
		basicShader->use();

		const float projectionX{ std::max(bgWidth, 10.f) };
		const float projectionY{ std::max(bgHeight, 10.f) };
		basicShader->setMatrix4("projection", Camera().getOffCenterOrthographicProjection(0, projectionX, 0, projectionY)); 
		
		if (backgroundTex.getID() > 0)
		{
			renderer->beginBatch();
			renderer->drawRectangle(Vector2(0, 0), Vector2(bgWidth, bgHeight), backgroundTex, 0, bgWidth, 0, bgHeight, backgroundDefaultTint, 0);
			renderer->endBatch();
			if (useDefaultTexture && IO::File::exists(jacket.getFilename()))
				updateDrawDefaultJacket(renderer, jacket);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		backgroundTex.dispose();
	}

	void ScorePreviewBackground::updateDrawDefaultJacket(Renderer* renderer, const Jacket& jacket)
	{
		if (jacket.getFilename().empty())
			return;

		int index = ResourceManager::getTexture("stage");
		if (index == -1)
			return;

		const Texture& stage = ResourceManager::textures[index];
		if (stage.sprites.size() < STAGE_SPR_COUNT)
			return;

		int shaderId;
		if ((shaderId = ResourceManager::getShader("basic2d")) == -1)
			return;
		Shader* basicShader = ResourceManager::shaders[shaderId];

		if ((shaderId = ResourceManager::getShader("masking")) == -1)
			return;
		Shader* maskShader = ResourceManager::shaders[shaderId];

		const DirectX::XMFLOAT4 defCol{ 1.f, 1.f, 1.f, 1.f };
		const DirectX::XMFLOAT4 mainCol{ 1.f, 1.f, 1.f, 0.65f };
		const DirectX::XMFLOAT4 mirrorCol{ 1.f, 1.f, 1.f, 0.35f };

		auto mainLeftPos = Engine::quadvPos(602.f, 602.f + 264.f, 816.f, 816.f + 174.f);
		auto mainRightPos = Engine::quadvPos(1205.f, 1205.f + 200.f, 629.f, 629.f + 114.f);
		auto mirrorLeftPos = Engine::quadvPos(615.f, 615.f + 256.f, 1170.f, 1170.f + 162.f);
		auto mirrorRightPos = Engine::quadvPos(1186.f, 1186.f + 196.f, 1387.f, 1387.f + 105.f);

		auto mainLeftMask = Engine::quadUV(stage.sprites[SPR_JACKET_LEFT_MASK], stage);
		auto mainRightMask = Engine::quadUV(stage.sprites[SPR_JACKET_RIGHT_MASK], stage);
		auto mirrorLeftMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_LEFT_MASK], stage);
		auto mirrorRightMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_RIGHT_MASK], stage);

		maskShader->use();
		maskShader->setInt("baseTex", 0);
		maskShader->setInt("maskTex", 1);
		maskShader->setMatrix4("projection", Camera().getOffCenterOrthographicProjection(0.f, 2048.f, 0.f, 2048.f));
		renderer->pushQuadMasked(mainLeftPos, DefaultJacket::getLeftUV(), mainLeftMask, mainCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mainRightPos, DefaultJacket::getRightUV(), mainRightMask, mainCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorLeftPos, DefaultJacket::getLeftMirrorUV(), mirrorLeftMask, mirrorCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorRightPos, DefaultJacket::getRightMirrorUV(), mirrorRightMask, mirrorCol, jacket.getTexID(), stage.getID());

		basicShader->use();
		basicShader->setMatrix4("projection", Camera().getOffCenterOrthographicProjection(0.f, 2048.f, 0.f, 2048.f));
		renderer->beginBatch();
		renderer->pushQuad(mainLeftPos, mainLeftMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mainRightPos, mainRightMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorLeftPos, mirrorLeftMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorRightPos, mirrorRightMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);

		const Sprite* sprite = &stage.sprites[SPR_JACKET_WINDOW];
		renderer->drawRectangle(Vector2(682.f, 497.f), Vector2(686.f, 686.f), stage,
		                        sprite->getX(), sprite->getX() + sprite->getWidth(),
		                        sprite->getY(), sprite->getY() + sprite->getHeight(), backgroundDefaultTint, 1);

		sprite = &stage.sprites[SPR_MIRROR_JACKET_WINDOW];
		renderer->drawRectangle(Vector2(699.f, 958.f), Vector2(651.f, 650.f), stage,
		                        sprite->getX(), sprite->getX() + sprite->getWidth(),
		                        sprite->getY(), sprite->getY() + sprite->getHeight(),
		                        Color(backgroundDefaultTint.r, backgroundDefaultTint.g, backgroundDefaultTint.b, backgroundDefaultTint.a * 0.6f), 1);
		renderer->endBatch();

		auto mainWindowPos = Engine::quadvPos(824.f, 824.f + 400.f, 666.f, 666.f + 384.f);
		auto mainWindowMask = Engine::quadUV(stage.sprites[SPR_JACKET_MASK], stage);
		auto mirrorWindowPos = Engine::quadvPos(834.f, 834.f + 386.f, 1120.f, 1120.f + 336.f);
		auto mirrorWindowMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_MASK], stage);

		maskShader->use();
		renderer->pushQuadMasked(mainWindowPos, DefaultJacket::getCenterUV(), mainWindowMask,
		                         { 1.f, 1.f, 1.f, 0.8f }, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorWindowPos, DefaultJacket::getMirrorCenterUV(), mirrorWindowMask,
		                         { 1.f, 1.f, 1.f, 0.5f }, jacket.getTexID(), stage.getID());

		basicShader->use();
		renderer->beginBatch();
		renderer->pushQuad(mainWindowPos, mainWindowMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorWindowPos, mirrorWindowMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);

		sprite = &stage.sprites[SPR_SEKAI_FLOOR];
		renderer->drawRectangle(Vector2(0.f, 1251.f), Vector2(sprite->getWidth(), sprite->getHeight()), stage,
		                        sprite->getX(), sprite->getX() + sprite->getWidth(),
		                        sprite->getY(), sprite->getY() + sprite->getHeight(),
		                        Color(backgroundDefaultTint.r, backgroundDefaultTint.g, backgroundDefaultTint.b, backgroundDefaultTint.a * 0.8f), 1);
		renderer->endBatch();
	}

	bool ScorePreviewBackground::shouldUpdate(const Jacket& jacket) const
	{
		return !init || backgroundFile != config.backgroundImage || jacketFile != jacket.getFilename();
	}

	void ScorePreviewBackground::draw(Renderer *renderer, float scrWidth, float scrHeight) const
	{
		float bgScrWidth = (float)frameBuffer.getWidth(), bgScrHeight = (float)frameBuffer.getHeight(), targetWidth, targetHeight;
		if (!backgroundFile.empty())
		{
			backgroundFillRect(scrWidth, scrHeight, bgScrWidth / bgScrHeight, bgScrWidth, bgScrHeight);
			targetWidth = Engine::STAGE_TARGET_WIDTH;
			targetHeight = Engine::STAGE_TARGET_HEIGHT;
		}
		else
		{
			bgScrWidth = Engine::BACKGROUND_SIZE;
			bgScrHeight = Engine::BACKGROUND_SIZE;
			targetWidth = Engine::STAGE_TARGET_WIDTH;
			targetHeight = Engine::STAGE_TARGET_HEIGHT;
		}
		
		const float bgWidth = bgScrWidth / (targetWidth * Engine::STAGE_WIDTH_RATIO);
		const float bgLeft = -bgWidth / 2.f;
		const float bgHeight = bgScrHeight / (targetHeight * Engine::STAGE_HEIGHT_RATIO);
		const float centerY = 0.5f / Engine::STAGE_HEIGHT_RATIO + Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float bgTop = centerY + -bgHeight / 2.f;
		auto vPos = Engine::quadvPos(bgLeft, bgLeft + bgWidth, bgTop, bgTop + bgHeight);
		auto uv = backgroundGetUV(0.f, 1.f, 0.f, 1.f);
		renderer->pushQuad(vPos, uv, DirectX::XMMatrixIdentity(), DirectX::XMFLOAT4(brightness, brightness, brightness, 1.f), frameBuffer.getTexture(), -10);
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getLeftUV() { return {{ { 303.8f / 740, 504.8f / 740, 0, 0 }, { 317.5f / 740, 297.7f / 740, 0, 0 }, { 5.5f / 740, 278.3f / 740, 0, 0 }, { -8.f / 740, 497.4f / 740, 0, 0 } }}; }
	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getRightUV() { return {{ { 749.5f / 740, 377.7f / 740, 0, 0 }, { 738.2f / 740, 188.1f / 740, 0, 0 }, { 415.0f / 740, 171.4f / 740, 0, 0 }, { 432.1f / 740, 363.9f / 740, 0, 0 } }}; }
	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getLeftMirrorUV() { return {{ { 292.761414f / 740, 247.401382f / 740, 0, 0 }, { 310.765869f / 740, 491.944763f / 740, 0, 0 }, { 6.892246f / 740, 498.470642f / 740, 0, 0 }, { -6.246704f / 740, 258.264862f / 740, 0, 0 } }}; }
	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getRightMirrorUV() { return {{ { 733.444458f / 740, 183.954681f / 740, 0, 0 }, { 743.541321f / 740, 355.960449f / 740, 0, 0 }, { 418.899414f / 740, 332.759491f / 740, 0, 0 }, { 410.746246f / 740, 155.907684f / 740, 0, 0 } }}; }
	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getCenterUV() { return {{ { 755.541687f / 740, 744.057861f / 740, 0, 0 }, { 739.961182f / 740, -1.859504f / 740, 0, 0 }, { 0.043696f / 740, -1.859504f / 740, 0, 0 }, { -17.484388f / 740, 744.057861f / 740, 0, 0 } }}; }
	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getMirrorCenterUV() { return {{ { 747.697083f / 740, 2.164453f / 740, 0, 0 }, { 743.909424f / 740, 731.297241f / 740, 0, 0 }, { -1.864066f / 740, 731.297241f / 740, 0, 0 }, { 3.837242f / 740, 2.164453f / 740, 0, 0 } }}; }

}
