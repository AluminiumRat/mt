#pragma once

#include <hld/FrameTypeIndex.h>

namespace mt
{
  class DrawPlan;
  class Camera;

  class DrawScene
  {
  public:
    DrawScene() noexcept = default;
    DrawScene(const DrawScene&) = delete;
    DrawScene& operator = (const DrawScene&) = delete;
    virtual ~DrawScene() noexcept = default;

    virtual void fillDrawPlan(DrawPlan& plan,
                              Camera& camera,
                              FrameTypeIndex frameTypeIndex) const = 0;
  };
}