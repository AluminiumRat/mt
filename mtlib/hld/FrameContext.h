#pragma once

#include <glm/glm.hpp>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class DrawPlan;
  class DrawStage;
  class FrameBuffer;

  class FrameContext
  {
  public:
    uint32_t frameTypeIndex;
    const DrawPlan* drawPlan;
    const DrawStage* drawStage;
    const FrameBuffer* frameBuffer;
    const Camera* viewCamera;
    CommandProducerGraphic* commandProducer;
  };
}