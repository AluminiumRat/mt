#pragma once

#include <glm/glm.hpp>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class DrawPlan;
  class FrameBuffer;

  class FrameContext
  {
  public:
    uint32_t frameTypeIndex;
    const DrawPlan* drawPlan;
    uint32_t drawStageIndex;
    const FrameBuffer* frameBuffer;
    const Camera* viewCamera;
    CommandProducerGraphic* commandProducer;
  };
}