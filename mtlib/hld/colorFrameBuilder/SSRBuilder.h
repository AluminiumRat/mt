#pragma once

#include <hld/colorFrameBuilder/HdrReprojector.h>
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
    void _createBuffers(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    ConstRef<ImageView> _reflectionBuffer;
    ConstRef<Image> _reprojectedHdr;
    ConstRef<ImageView> _prevHDRBuffer;

    HdrReprojector _hdrReprojector;;

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
    if( _reflectionBuffer == &reflectionBuffer &&
        _prevHDRBuffer == &prevHDRBuffer) return;

    try
    {
      _reprojectedHdr = nullptr;
      _prevHDRBuffer = &prevHDRBuffer;
      _reflectionBuffer = &reflectionBuffer;
      _reflectionBufferBinding.setImage(&reflectionBuffer);
      _prevHDRBufferBinding.setImage(&prevHDRBuffer);

      _gridSize = (glm::uvec2(reflectionBuffer.extent()) + glm::uvec2(7)) / 8u;
    }
    catch(...)
    {
      _reflectionBuffer = nullptr;
      _reflectionBufferBinding.setImage(nullptr);
      _prevHDRBuffer = nullptr;
      _prevHDRBufferBinding.setImage(nullptr);
      _reprojectedHdr = nullptr;
      throw;
    }
  }
}