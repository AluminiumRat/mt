#pragma once

#include <gui/RenderWindow.h>
#include <technique/Technique.h>
#include <util/Ref.h>

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
    void _createVertexBuffer();
    void _createTexture();

  private:
    Ref<Image> _renderedImage;

    struct ComputeTechnique
    {
      Ref<TechniqueConfigurator> configurator;
      Technique technique;
      TechniquePass& pass;
      ResourceBinding& texture;
    };
    ComputeTechnique _computeTechnique;

    struct RenderTechnique
    {
      Ref<TechniqueConfigurator> configurator;
      Technique technique;
      TechniquePass& pass;

      Selection& colorSelector;

      ResourceBinding& vertexBuffer;
      ResourceBinding& texture;

      UniformVariable& color;
    };
    RenderTechnique _renderTechnique;
  };
}