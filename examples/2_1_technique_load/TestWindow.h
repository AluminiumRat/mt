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
    void _createVertexBuffers(Device& device);
    void _createTextures(Device& device);

  private:
    Ref<Technique> _technique;

    TechniquePass& _pass1;
    TechniquePass& _pass2;

    Selection& _selector1;
    Selection& _selector2;

    Ref<DataBuffer> _vertexBuffers[2];
    TechniqueResource& _vertexBuffer;

    ConstRef<ImageView> _textures[2];
    TechniqueResource& _texture1;
    TechniqueResource& _texture2;

    //Ref<Sampler> _sampler;
    //TechniqueResource& _samplerResource;

    TechniqueResource& _unusedResource;

    UniformVariable& _color;
    UniformVariable& _shift;
  };
}