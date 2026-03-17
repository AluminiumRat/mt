#pragma once

#include <glm/glm.hpp>

#include <hld/FrameTypeIndex.h>

namespace mt
{
  class Camera;
  class DrawScene;

  struct FrameBuildContext
  {
    FrameTypeIndex frameType;
    const Camera* viewCamera;
    const DrawScene* drawScene;

    //  Размер области, в которую отрисовывается конечная картинка
    glm::uvec2 frameExtent;
  };
}