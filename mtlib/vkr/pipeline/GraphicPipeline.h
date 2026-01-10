#pragma once

#include <util/Ref.h>
#include <vkr/pipeline/AbstractPipeline.h>

namespace mt
{
  class FrameBufferFormat;

  //  Конфигурация графического конвейера. То есть то, каким именно образом
  //  будут обрабатываться данные на GPU, для того чтобы отрисовать что-то.
  class GraphicPipeline : public AbstractPipeline
  {
  public:
    GraphicPipeline(
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
  };
}