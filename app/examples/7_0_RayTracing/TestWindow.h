#pragma once

#include <gui/RenderWindow.h>
#include <util/Ref.h>
#include <vkr/accelerationStructure/TLAS.h>

namespace mt
{
  class TestWindow : public RenderWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void drawImplementation(FrameBuffer& frameBuffer) override;

  private:
    Ref<TLAS> _tlas;
  };
}