#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>
#include <GLFWRenderWindow.h>

namespace mt
{
  class TechniqueTestWindow : public GLFWRenderWindow
  {
  public:
    TechniqueTestWindow(Device& device);
    TechniqueTestWindow(const GLFWRenderWindow&) = delete;
    TechniqueTestWindow& operator = (const TechniqueTestWindow&) = delete;
    virtual ~TechniqueTestWindow() noexcept = default;

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    void _createVertexBuffers(Device& device);
    void _createTextures(Device& device);

  private:
    Ref<Technique> _technique;

    Selection& _selector1;
    Selection& _selector2;

    Ref<DataBuffer> _vertexBuffers[2];
    TechniqueResource& _vertexBuffer;

    ConstRef<ImageView> _textures[2];
    TechniqueResource& _texture1;
    TechniqueResource& _texture2;

    Ref<Sampler> _sampler;
    TechniqueResource& _samplerResource;

    TechniqueResource& _unusedResource;

    UniformVariable& _color;
    UniformVariable& _shift;
  };
}