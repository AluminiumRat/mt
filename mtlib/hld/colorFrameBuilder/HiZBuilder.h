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

  //  Штука, которая строит иерархический буфер глубины по буферу линейной
  //  глубины
  class HiZBuilder
  {
  public:
    explicit HiZBuilder(Device& device);
    HiZBuilder(const HiZBuilder&) = delete;
    HiZBuilder& operator = (const HiZBuilder&) = delete;
    ~HiZBuilder() noexcept = default;

    // hiZ должен находиться в VK_LAYOUT_GENERAL
    void buildHiZ(CommandProducerCompute& producer);

    inline void setBuffers(const Image& hiZ);

    //  Посчитать размер нулевого мипа для HiZ по размеру буфера глубины
    //  Просто округление вниз до степени двойки
    inline static glm::uvec2 getHiZExtent(
                                        glm::uvec2 linearDepthExtent) noexcept;

  private:
    Technique _technique;
    TechniquePass& _buildPass;
    ResourceBinding& _hiZBinding;
    UniformVariable& _hizSizeUniform;

    ConstRef<Image> _hiZ;

    //  Размер сетки для вызова компьют шейдера
    glm::uvec2 _gridSize;
  };

  inline void HiZBuilder::setBuffers(const Image& hiZ)
  {
    if(_hiZ == &hiZ) return;

    MT_ASSERT(hiZ.mipmapCount() == Image::calculateMipNumber(hiZ.extent()));

    std::vector<ConstRef<ImageView>> hiZViews;
    for(uint32_t mip = 0; mip < hiZ.mipmapCount(); mip++)
    {
      hiZViews.push_back(ConstRef(new ImageView(hiZ,
                                                ImageSlice(
                                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                                    mip),
                                                VK_IMAGE_VIEW_TYPE_2D)));
    }
    _hiZBinding.setImages(hiZViews);

    _hiZ = &hiZ;
    _gridSize = (glm::uvec2(_hiZ->extent()) + glm::uvec2(15)) / 16u;
    _hizSizeUniform.setValue(glm::uvec2(_hiZ->extent()));
  }

  inline glm::uvec2 HiZBuilder::getHiZExtent(
                                          glm::uvec2 linearDepthExtent) noexcept
  {
    return floorPow(linearDepthExtent);
  }
}