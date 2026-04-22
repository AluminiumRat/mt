#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  //  Штука, которая делает репроекцию HDR буфера из предыдущего кадра в
  //  текущий кадр и строит набор мипов для неё. Так же делает размытие
  //  полученных мипов
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
    void make(CommandProducerCompute& producer);

    //  reprojectedHdr должен содержать не более 10 мип уровней
    //  reprojectedHdr должен иметь размерность кратную степени 2
    inline void setBuffers( const Image& reprojectedHdr,
                            const ImageView& prevHDRBuffer);

  private:
    void _createPingpongBuffer(CommandProducerCompute& producer);
    void _makeMips( CommandProducerCompute& producer,
                    const char* baseMipValue,
                    glm::uvec2 gridSize);
    void _filterHorizontal( CommandProducerCompute& producer,
                            uint32_t baseMip,
                            glm::uvec2 gridSize);
    void _filterVertical( CommandProducerCompute& producer,
                          uint32_t baseMip,
                          glm::uvec2 gridSize);

  private:
    Device& _device;

    Technique _reprojectTechnique;
    TechniquePass& _reprojectPass;
    ResourceBinding& _prevHdrBinding;
    ResourceBinding& _reprojectedHdrBinding;
    UniformVariable& _targetSizeUniform;
    UniformVariable& _targetMipCountUniform;
    Selection& _baseMipSelection;
    //  Размер сетки компьюта при репроекции и построении мипов
    glm::uvec2 _reprojectGridSize;

    Technique _filterTechnique;
    TechniquePass& _filterHorizontalPass;
    TechniquePass& _filterVerticalPass;
    ResourceBinding& _filterSourceBinding;
    ResourceBinding& _filterTargetBinding;
    //  Размер сетки компьюта при фильтрации
    glm::uvec2 _filterGridSize;

    ConstRef<Image> _reprojectedHdr;
    std::vector<ConstRef<ImageView>> _reprojectedHdrViews;

    //  Буфер для временного хранения промежуточных результатов фильтрации
    ConstRef<Image> _pinpongBuffer;
    std::vector<ConstRef<ImageView>> _pinpongViews;
  };

  inline void HdrReprojector::setBuffers( const Image& reprojectedHdr,
                                          const ImageView& prevHDRBuffer)
  {
    MT_ASSERT(reprojectedHdr.mipmapCount() <= 10);
    if( _reprojectedHdr == &reprojectedHdr &&
        _prevHdrBinding.image() == &prevHDRBuffer) return;

    try
    {
      _pinpongBuffer.reset();
      _pinpongViews.clear();
      _reprojectedHdrViews.clear();
      _prevHdrBinding.setImage(&prevHDRBuffer);

      for(uint32_t mip = 0; mip < reprojectedHdr.mipmapCount(); mip++)
      {
        _reprojectedHdrViews.push_back(
                          ConstRef(new ImageView( reprojectedHdr,
                                                  ImageSlice(
                                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                                      mip),
                                                  VK_IMAGE_VIEW_TYPE_2D)));
      }
      _reprojectedHdrBinding.setImages(_reprojectedHdrViews);

      _reprojectGridSize =
                  (glm::uvec2(reprojectedHdr.extent()) + glm::uvec2(15)) / 16u;
      _targetSizeUniform.setValue(glm::uvec2(reprojectedHdr.extent()));
      _targetMipCountUniform.setValue(reprojectedHdr.mipmapCount());

      _reprojectedHdr = &reprojectedHdr;

      _filterGridSize =
                  (glm::uvec2(_reprojectedHdr->extent()) + glm::uvec2(7)) / 8u;
    }
    catch(...)
    {
      _pinpongBuffer.reset();
      _pinpongViews.clear();
      _prevHdrBinding.setImage(nullptr);
      _reprojectedHdrBinding.setImage(nullptr);
      _reprojectedHdr = nullptr;
      _reprojectedHdrViews.clear();
      throw;
    }
  }
}