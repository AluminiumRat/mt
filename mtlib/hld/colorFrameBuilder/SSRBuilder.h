#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  class SSRBuilder
  {
  public:
    explicit SSRBuilder(Device& device);
    SSRBuilder(const SSRBuilder&) = delete;
    SSRBuilder& operator = (const SSRBuilder&) = delete;
    ~SSRBuilder() noexcept = default;

    //  reflectionBuffer должен находиться в VK_IMAGE_LAYOUT_GENERAL
    //  prevHDRBuffer должен находиться в
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void buildReflection(CommandProducerGraphic& commandProducer);

    inline void setBuffers( const ImageView& reflectionBuffer,
                            const ImageView& prevHDRBuffer);

  private:
    Device& _device;

    Technique _technique;
    TechniquePass& _marchingPass;
    ResourceBinding& _reflectionBufferBinding;
    ResourceBinding& _prevHDRBufferBinding;

    //  Размер сетки для вызова компьют шейдеров
    glm::uvec2 _gridSize;
  };

  inline void SSRBuilder::setBuffers( const ImageView& reflectionBuffer,
                                      const ImageView& prevHDRBuffer)
  {
    if( _reflectionBufferBinding.image() == &reflectionBuffer &&
        _prevHDRBufferBinding.image() == &prevHDRBuffer) return;

    _reflectionBufferBinding.setImage(&reflectionBuffer);
    _prevHDRBufferBinding.setImage(&prevHDRBuffer);

    _gridSize = (glm::uvec2(reflectionBuffer.extent()) + glm::uvec2(7)) / 8u;
  }
}