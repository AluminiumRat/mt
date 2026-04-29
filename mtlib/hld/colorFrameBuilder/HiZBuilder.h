#pragma once

#include <technique/Technique.h>
#include <util/Assert.h>
#include <util/floorPow.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  //  Штука, которая строит иерархический буфер глубины и буфер наполнения по буферу
  //  линейной глубины.
  class HiZBuilder
  {
  public:
    explicit HiZBuilder(Device& device);
    HiZBuilder(const HiZBuilder&) = delete;
    HiZBuilder& operator = (const HiZBuilder&) = delete;
    ~HiZBuilder() noexcept = default;

    // hiZ и fullnessBuffer должны находиться в VK_LAYOUT_GENERAL
    void buildHiZ(CommandProducerCompute& producer);

    //  Максимум 15 мипов
    inline void setBuffers( const Image& hiZ,
                            const Image& fullnessBuffer);

    //  Посчитать размер нулевого мипа для HiZ по размеру буфера глубины
    //  Просто округление вниз до степени двойки
    inline static glm::uvec2 getHiZExtent(
                                        glm::uvec2 linearDepthExtent) noexcept;

  private:
    void _barriers( CommandProducerCompute& producer,
                    uint32_t baseMip);

  private:
    Technique _technique;
    TechniquePass& _buildPass;
    ResourceBinding& _hiZBinding;
    ResourceBinding& _fullnessBinding;
    UniformVariable& _hizSizeUniform;
    UniformVariable& _hizMipCountUniform;
    Selection& _baseMipSelection;

    ConstRef<Image> _hiZ;
    ConstRef<Image> _fullnessBuffer;

    //  Размер сетки для вызова компьют шейдера
    glm::uvec2 _gridSize;
  };

  inline void HiZBuilder::setBuffers( const Image& hiZ,
                                      const Image& fullnessBuffer)
  {
    if(_hiZ == &hiZ && _fullnessBuffer == &fullnessBuffer) return;

    MT_ASSERT(hiZ.mipmapCount() == Image::calculateMipNumber(hiZ.extent()));
    MT_ASSERT(hiZ.mipmapCount() == fullnessBuffer.mipmapCount());
    MT_ASSERT(hiZ.mipmapCount() <= 15)

    try
    {
      std::vector<ConstRef<ImageView>> hiZViews;
      std::vector<ConstRef<ImageView>> fullnessViews;
      for(uint32_t mip = 0; mip < 15; mip++)
      {
        hiZViews.push_back(ConstRef(new ImageView(hiZ,
                                                  ImageSlice(
                                                      hiZ,
                                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                                      mip,
                                                      1),
                                                  VK_IMAGE_VIEW_TYPE_2D)));
        fullnessViews.push_back(ConstRef(new ImageView(
                                                  fullnessBuffer,
                                                  ImageSlice(
                                                      fullnessBuffer,
                                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                                      mip,
                                                      1),
                                                  VK_IMAGE_VIEW_TYPE_2D)));
      }
      _hiZBinding.setImages(hiZViews);
      _fullnessBinding.setImages(fullnessViews);

      _gridSize = (glm::uvec2(hiZ.extent()) + glm::uvec2(15)) / 16u;
      _hizSizeUniform.setValue(glm::uvec2(hiZ.extent()));
      _hizMipCountUniform.setValue(hiZ.mipmapCount());

      _hiZ = &hiZ;
      _fullnessBuffer = &fullnessBuffer;
    }
    catch(...)
    {
      _hiZ.reset();
      _hiZBinding.setImage(nullptr);
      _fullnessBuffer.reset();
      _fullnessBinding.setImage(nullptr);
      throw;
    }
  }

  inline glm::uvec2 HiZBuilder::getHiZExtent(
                                          glm::uvec2 linearDepthExtent) noexcept
  {
    return floorPow(linearDepthExtent);
  }
}