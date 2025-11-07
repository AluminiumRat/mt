#pragma once

#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/Ref.h>

namespace mt
{
  class FrameBufferFormat;

  //  Конфигурация графического конвейера. То есть то, каким именно образом
  //  будут обрабатываться данные на GPU, для того чтобы отрисовать что-то.
  class GraphicPipeline : public AbstractPipeline
  {
  public:
    GraphicPipeline(
              Device& device,
              const FrameBufferFormat& frameBufferFormat,
              std::span<const ShaderInfo> shaders,
              VkPrimitiveTopology topology,
              const VkPipelineRasterizationStateCreateInfo& rasterizationState,
              const VkPipelineDepthStencilStateCreateInfo& depthStencilState,
              const VkPipelineColorBlendStateCreateInfo& blendingState,
              const PipelineLayout& layout);
    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline& operator = (const GraphicPipeline&) = delete;
    virtual ~GraphicPipeline() noexcept = default;

  private:
    ConstRef<PipelineLayout> _layout;
  };
}