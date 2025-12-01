#pragma once

#include <imgui.h>

#include <vkr/GLFWRenderWindow.h>

namespace mt
{
  class TestWindow : public GLFWRenderWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const GLFWRenderWindow&) = delete;
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