#include "ScorePreview.h"
#include "../Application.h"
#include "../ApplicationConfiguration.h"
#include "../IO.h"
#include "../ResourceManager.h"
#include "../Utilities.h"

namespace MikuMikuWorld
{
	void ScorePreviewWindow::loadNoteEffects(Effect::EffectView& effectView)
	{
		int oldProfile = config.pvEffectsProfile == 1 ? 0 : 1;
		const std::string oldEffectsDir = Application::getAppDir() + "res\\effect\\" + std::to_string(oldProfile) + "\\";
		const std::string effectsDir = Application::getAppDir() + "res\\effect\\" + std::to_string(config.pvEffectsProfile) + "\\";
		size_t effectCount = arrayLength(Effect::effectNames);

		ResourceManager::removeAllParticleEffects();
		int texIndex = ResourceManager::getTextureByFilename(oldEffectsDir + "tex_note_common_all_v2.png");
		if (texIndex > -1)
			ResourceManager::disposeTexture(ResourceManager::textures[texIndex].getID());

		ResourceManager::loadTexture(effectsDir + "tex_note_common_all_v2.png");

		std::vector<std::string> failedParticleFiles;
		for (size_t i = 0; i < effectCount; i++)
		{
			const std::string filename{ effectsDir + Effect::effectNames[i] + ".json" };
			int particleId = ResourceManager::loadParticleEffect(filename);
			if (particleId == -1)
				failedParticleFiles.push_back(filename);
		}

		if (!failedParticleFiles.empty())
		{
			std::string fullErrorMessage = "Failed to load the following note effects: \n\n";
			for (const auto& error : failedParticleFiles)
				fullErrorMessage.append(error).append("\n");

			IO::messageBox(
				APP_NAME,
				fullErrorMessage,
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Warning,
				Application::windowState.windowHandle
			);
		}

		effectView.reset();
		effectView.init();
	}
}
