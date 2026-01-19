#pragma once

#include <glm/glm.hpp>

#include <hld/FrameTypeIndex.h>

namespace mt
{
  class Camera;
  class CommandMemoryPool;
  class CommandProducerGraphic;
  class DrawPlan;
  class FrameBuffer;

  class FrameContext
  {
  public:
    FrameTypeIndex frameTypeIndex;
    const DrawPlan* drawPlan;
    CommandMemoryPool* commandMemoryPool;
    uint32_t drawStageIndex;
    const FrameBuffer* frameBuffer;
    const Camera* viewCamera;
    CommandProducerGraphic* commandProducer;
  };
}