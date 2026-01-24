#pragma once

#include <functional>

#include <vulkan/vulkan.h>

#include <hld/colorFrameBuilder/OpaqueColorStage.h>
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
  struct ColorFrameContext;
  class CommandProducerGraphic;
  class Device;
  class DrawScene;
  class GlobalLight;

  class ColorFrameBuilder
  {
  public:
    static constexpr const char* frameTypeName = "ColorFrame";
    static constexpr uint32_t cameraBufferBinding = 0;
    static constexpr uint32_t illuminationBufferBinding = 1;

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
                      const ExtraDraw& imGuiDraw,
                      CommandProducerGraphic& commandProducer);

  private:
    void _updateBuffers(FrameBuffer& targetFrameBuffer);
    Ref<DescriptorSet> _buildCommonSet(ColorFrameContext& context);

  private:
    Device& _device;

    DrawPlan _drawPlan;
    CommandMemoryPool _memoryPool;

    ConstRef<DescriptorSetLayout> _commonSetLayout;
    ConstRef<PipelineLayout> _commonSetPipelineLayout;
    ConstRef<DataBuffer> _cameraBuffer;

    FrameTypeIndex _frameTypeIndex;
    Ref<Image> _hdrBuffer;
    Ref<Image> _depthBuffer;

    OpaqueColorStage _opaqueColorStage;
  };
}