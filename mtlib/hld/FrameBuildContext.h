#pragma once

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
  };
}