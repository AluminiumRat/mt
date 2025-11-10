#pragma once

#include <util/Ref.h>
#include <GLFWRenderWindow.h>
#include <vkr/pipeline/GraphicPipeline.h>

namespace mt
{
  class TestWindow : public GLFWRenderWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const GLFWRenderWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    Ref<GraphicPipeline> _createPipeline();

  private:
    Ref<GraphicPipeline> _pipeline;
  };
}