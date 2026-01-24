#pragma once

#include <glm/glm.hpp>

#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class DrawPlan;
  class FrameBuffer;

  class FrameContext
  {
  public:
    const DrawPlan* drawPlan;
    const Camera* viewCamera;
    CommandProducerGraphic* commandProducer;
  };
}