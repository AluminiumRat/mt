#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <hld/drawScene/DrawScene.h>

namespace mt
{
  class CommandQueueGraphic;
  class TechniqueManager;
  class TextureManager;

  void loadGLBScene(std::filesystem::path file,
                    DrawScene& targetScene,
                    std::vector<std::unique_ptr<Drawable>>& drawablesContainer,
                    CommandQueueGraphic& uploadingQueue,
                    TextureManager& textureManager,
                    TechniqueManager& techniqueManager);
}