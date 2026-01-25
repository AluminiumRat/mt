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
    virtual void drawImplementation(FrameBuffer& frameBuffer) override;

  private:
    void _makeConfiguration();
    void _createVertexBuffer();
    void _createTexture();

  private:
    Ref<TechniqueConfigurator> _configurator;
    Ref<Technique> _technique;
    TechniquePass& _pass;
    ResourceBinding& _vertexBuffer;
    ResourceBinding& _texture;
  };
}