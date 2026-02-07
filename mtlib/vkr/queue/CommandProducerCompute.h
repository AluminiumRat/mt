#pragma once

#include <vector>

#include <util/Ref.h>
#include <vkr/pipeline/ComputePipeline.h>
#include <vkr/queue/CommandProducerTransfer.h>

namespace mt
{
  //  Продюсер команд, реализующий функционал компьют очереди
  //  Так же включает в себя функционал CommandProducerTransfer
  class CommandProducerCompute : public CommandProducerTransfer
  {
  public:
    //  Если debugName - не пустая строка, то все команды продюсера
    //  будут обернуты в vkCmdBeginDebugUtilsLabelEXT и
    //  vkCmdEndDebugUtilsLabelEXT
    CommandProducerCompute(CommandPoolSet& poolSet, const char* debugName);
    CommandProducerCompute(const CommandProducerCompute&) = delete;
    CommandProducerCompute& operator = (const CommandProducerCompute&) = delete;
    virtual ~CommandProducerCompute() noexcept = default;

    void bindComputePipeline(const ComputePipeline& pipeline);
    void unbindComputePipeline();

    void bindDescriptorSetCompute(const DescriptorSet& descriptorSet,
                                  uint32_t setIndex,
                                  const PipelineLayout& layout);

    void unbindDescriptorSetCompute(uint32_t setIndex) noexcept;

    //  Запустить прибинженный пайплайн
    void dispatch(uint32_t gridSizeX, uint32_t gridSizeY, uint32_t gridSizeZ);
    inline void dispatch(glm::uvec3 gridSize);
    inline void dispatch(uint32_t gridSizeX, uint32_t gridSizeY);
    inline void dispatch(glm::uvec2 gridSize);
    inline void dispatch(uint32_t gridSize);

  protected:
    virtual void finalizeCommands() noexcept override;
    virtual void restoreBindings(CommandBuffer& buffer) override;

  private:
    struct BindingRecord
    {
      ConstRef<PipelineLayout> layout;
      ConstRef<DescriptorSet> descriptors;
    };

  private:
    ConstRef<ComputePipeline> _computePipeline;
    std::vector<BindingRecord> _computeDescriptors;
  };

  inline void CommandProducerCompute::dispatch(glm::uvec3 gridSize)
  {
    dispatch(gridSize.x, gridSize.y, gridSize.z);
  }

  inline void CommandProducerCompute::dispatch( uint32_t gridSizeX,
                                                uint32_t gridSizeY)
  {
    dispatch(gridSizeX, gridSizeY, 1);
  }
  
  inline void CommandProducerCompute::dispatch(glm::uvec2 gridSize)
  {
    dispatch(gridSize.x, gridSize.y, 1);
  }

  inline void CommandProducerCompute::dispatch(uint32_t gridSize)
  {
    dispatch(gridSize, 1, 1);
  }
}