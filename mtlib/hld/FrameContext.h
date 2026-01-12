#pragma once

#include <glm/glm.hpp>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class DrawStage;
  class FrameBuffer;

  class FrameContext
  {
  public:
    const DrawStage* drawStage;
    const FrameBuffer* frameBuffer;
    const Camera* viewCamera;
    CommandProducerGraphic* commandProducer;
  };
}