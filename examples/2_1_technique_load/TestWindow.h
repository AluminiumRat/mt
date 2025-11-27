#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/GLFWRenderWindow.h>
#include <vkr/Sampler.h>

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
    void _makeConfiguration();
    void _createVertexBuffer();
    void _createTexture();

  private:
    Ref<Technique> _technique;

    TechniquePass& _pass;

    Selection& _colorSelector;

    TechniqueResource& _vertexBuffer;
    TechniqueResource& _texture;

    UniformVariable& _color;
  };
}