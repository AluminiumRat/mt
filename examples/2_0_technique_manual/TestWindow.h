#pragma once

#include <gui/RenderWindow.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>

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
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    void _makeConfiguration();
    void _createVertexBuffer();
    void _createTexture();
    void _drawSimple(CommandProducerGraphic& commandProducer);
    void _drawVolatileContext(CommandProducerGraphic& commandProducer) const;

  private:
    Ref<Technique> _technique;

    TechniquePass& _pass;

    Selection& _colorSelector;

    ResourceBinding& _vertexBuffer;
    ResourceBinding& _texture;
    ResourceBinding& _sampler;

    UniformVariable& _color;
  };
}