#pragma once

#include <glm/glm.hpp>

#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>

namespace mt
{
  class Camera;
  class DrawPlan;

  class FrameContext
  {
  public:
    const DrawPlan* drawPlan;
    const Camera* viewCamera;
  };
}