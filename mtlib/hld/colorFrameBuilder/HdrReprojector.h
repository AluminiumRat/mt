#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  //  Штука, которая делает репроекцию HDR буфера из предыдущего кадра в
  //  текущий кадр и строит набор мипов для неё
  class HdrReprojector
  {
  public:
    explicit HdrReprojector(Device& device);
    HdrReprojector(const HdrReprojector&) = delete;
    HdrReprojector& operator = (const HdrReprojector&) = delete;
    ~HdrReprojector() noexcept = default;

    //  reprojectedHdr должен находиться в VK_LAYOUT_GENERAL
    //  prevHDRBuffer должен находиться в
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void reproject(CommandProducerCompute& producer);

    inline void setBuffers( const Image& reprojectedHdr,
                            const ImageView& prevHDRBuffer);

  private:
    Technique _technique;
    TechniquePass& _reprojectPass;
    ResourceBinding& _prevHdrBinding;
    ResourceBinding& _reprojectedHdrBinding;
    UniformVariable& _targetSizeUniform;
    UniformVariable& _targetMipCountUniform;
    Selection& _baseMipSelection;

    ConstRef<Image> _reprojectedHdr;

    //  Размер сетки компьюта при репроекции и построении мипов
    glm::uvec2 _gridSize;
  };

  inline void HdrReprojector::setBuffers( const Image& reprojectedHdr,
                                          const ImageView& prevHDRBuffer)
  {
    if( _reprojectedHdr == &reprojectedHdr &&
        _prevHdrBinding.image() == &prevHDRBuffer) return;

    try
    {
      _prevHdrBinding.setImage(&prevHDRBuffer);

      std::vector<ConstRef<ImageView>> views;
      for(uint32_t mip = 0; mip < reprojectedHdr.mipmapCount(); mip++)
      {
        views.push_back(ConstRef(new ImageView( reprojectedHdr,
                                                ImageSlice(
                                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                                    mip),
                                                VK_IMAGE_VIEW_TYPE_2D)));
      }
      _reprojectedHdrBinding.setImages(views);

      _gridSize = (glm::uvec2(reprojectedHdr.extent()) + glm::uvec2(15)) / 16u;
      _targetSizeUniform.setValue(glm::uvec2(reprojectedHdr.extent()));
      _targetMipCountUniform.setValue(reprojectedHdr.mipmapCount());

      _reprojectedHdr = &reprojectedHdr;
    }
    catch(...)
    {
      _prevHdrBinding.setImage(nullptr);
      _reprojectedHdrBinding.setImage(nullptr);
      _reprojectedHdr = nullptr;
      throw;
    }
  }
}