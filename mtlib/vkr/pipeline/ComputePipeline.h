#pragma once

#include <vkr/pipeline/AbstractPipeline.h>

namespace mt
{
  //  Конфигурация компьют конвейера. То есть то, каким именно образом
  //  будут обрабатываться данные на GPU, для того чтобы посчитать что-то.
  class ComputePipeline : public AbstractPipeline
  {
  public:
    ComputePipeline(std::span<const ShaderInfo> shaders,
                    const PipelineLayout& layout);
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator = (const ComputePipeline&) = delete;
    virtual ~ComputePipeline() noexcept = default;
  };
}