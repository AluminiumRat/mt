#pragma once

#include <functional>

#include <vulkan/vulkan.h>

#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <hld/colorFrameBuilder/Posteffects.h>
#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/DrawPlan.h>
#include <hld/FrameTypeIndex.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
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
  class GlobalLight;
  class TechniqueManager;

  class ColorFrameBuilder
  {
  public:
    static constexpr const char* frameTypeName = "ColorFrame";

    static constexpr VkFormat hdrFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    static constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

  public:
    using ExtraDraw =
                  std::function<void(CommandProducerGraphic& commandProducer)>;

  public:
    explicit ColorFrameBuilder(Device& device);
    ColorFrameBuilder(const ColorFrameBuilder&) = delete;
    ColorFrameBuilder& operator = (const ColorFrameBuilder&) = delete;
    virtual ~ColorFrameBuilder() noexcept = default;

  public:
    virtual void draw(FrameBuffer& target,
                      const DrawScene& scene,
                      const Camera& viewCamera,
                      const GlobalLight& illumination,
                      const ExtraDraw& imGuiDraw);

    //  Добавить окно с настройками в текущий контекст ImGui
    void makeGui();

  private:
    void _updateBuffers(FrameBuffer& targetFrameBuffer);
    void _initBuffersLayout(CommandProducerGraphic& commandProducer);
    void _posteffectsPrepareLayouts(CommandProducerGraphic& commandProducer);
    void _posteffectsResolveLayouts(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    DrawPlan _drawPlan;

    FrameTypeIndex _frameTypeIndex;
    Ref<Image> _hdrBuffer;
    Ref<Image> _depthBuffer;

    ColorFrameCommonSet _commonSet;

    OpaqueColorStage _opaqueColorStage;
    Posteffects _posteffects;
  };
}