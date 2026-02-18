#pragma once

#include <functional>

#include <vulkan/vulkan.h>

#include <hld/colorFrameBuilder/BackgroundRender.h>
#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <hld/colorFrameBuilder/Posteffects.h>
#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/DrawPlan.h>
#include <hld/FrameTypeIndex.h>
#include <util/Ref.h>
#include <util/Region.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImageView.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/DataBuffer.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class Device;
  class DrawScene;
  class EnvironmentScene;
  class TechniqueManager;
  class TextureManager;

  //  Штуковина для отрисовки сцены в цветовой кадр. То есть то, что должно
  //  показываться пользователю
  class ColorFrameBuilder
  {
  public:
    static constexpr const char* frameTypeName = "ColorFrame";

    static constexpr VkFormat hdrFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    static constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

  public:
    explicit ColorFrameBuilder( Device& device,
                                TextureManager& textureManager,
                                TechniqueManager& techniqueManager);
    ColorFrameBuilder(const ColorFrameBuilder&) = delete;
    ColorFrameBuilder& operator = (const ColorFrameBuilder&) = delete;
    virtual ~ColorFrameBuilder() noexcept = default;

  public:
    virtual void draw(FrameBuffer& target,
                      const DrawScene& scene,
                      const Camera& viewCamera,
                      const EnvironmentScene& environment);

    //  Регион таргета, в который отрисовываться изображение.
    //  Если регион невалидный, то отрисовка производится в весь таргет
    inline const Region& drawRegion() const noexcept;
    //  Выставить регион, в который будет отрисовываться изображение
    //  Если регион невалидный, то отрисовка производится в весь таргет
    //  ВНИМАНИЕ!!! Изменение региона будет приводить к перевыделению внутренних
    //    буферов и сбросу истории.
    inline void setDrawRegion(const Region& newValue) noexcept;

    //  Добавить окно с настройками в текущий контекст ImGui
    void makeGui();

  private:
    void _updateBuffers(glm::uvec2 targetExtent);
    void _initBuffersLayout(CommandProducerGraphic& commandProducer);
    void _posteffectsLayouts(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    FrameTypeIndex _frameTypeIndex;

    Region _drawRegion;

    DrawPlan _drawPlan;

    Ref<Image> _hdrBuffer;
    Ref<ImageView> _hdrBufferView;
    Ref<Image> _depthBuffer;
    Ref<ImageView> _depthBufferView;

    ColorFrameCommonSet _commonSet;

    OpaqueColorStage _opaqueColorStage;
    BackgroundRender _backgroundRender;
    Posteffects _posteffects;
  };

  inline const Region& ColorFrameBuilder::drawRegion() const noexcept
  {
    return _drawRegion;
  }
  
  inline void ColorFrameBuilder::setDrawRegion(const Region& newValue) noexcept
  {
    _drawRegion = newValue;
  }
}