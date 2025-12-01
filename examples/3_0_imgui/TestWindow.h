#pragma once

#include <imgui.h>

#include <gui/RenderWindow.h>

namespace mt
{
  class TestWindow : public RenderWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept;

    virtual void update();

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    ImGuiContext* _imguiContext;
    ImGuiIO* _imGuiIO;
  };
}