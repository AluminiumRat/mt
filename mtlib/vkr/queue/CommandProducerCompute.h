#pragma once

#include <vkr/image/ImageAccessMultiset.h>
#include <vkr/queue/CommandProducerTransfer.h>

namespace mt
{
  class ComputePipeline;
  class PipelineLayout;

  //  Продюсер команд, реализующий функционал компьют очереди
  //  Так же включает в себя функционал CommandProducerTransfer
  class CommandProducerCompute : public CommandProducerTransfer
  {
  public:
    //  Максимальное количество дескриптер сетов, которые можно прибиндить к
    //  компьют пайплайну
    static constexpr uint32_t maxComputeDescriptorSetsNumber =
                                              ImageAccessMultiset::maxSetsCount;

  public:
    //  Если debugName - не пустая строка, то все команды продюсера
    //  будут обернуты в vkCmdBeginDebugUtilsLabelEXT и
    //  vkCmdEndDebugUtilsLabelEXT
    CommandProducerCompute(CommandPoolSet& poolSet, const char* debugName);
    CommandProducerCompute(const CommandProducerCompute&) = delete;
    CommandProducerCompute& operator = (const CommandProducerCompute&) = delete;
    virtual ~CommandProducerCompute() noexcept = default;

    void setComputePipeline(const ComputePipeline& pipeline);

    void bindDescriptorSetCompute(const DescriptorSet& descriptorSet,
                                  uint32_t setIndex,
                                  const PipelineLayout& layout);

    void unbindDescriptorSetCompute(uint32_t setIndex) noexcept;

    //  Запустить прибинженный пайплайн
    void dispatch(uint32_t x, uint32_t y, uint32_t z);

  private:
    ImageAccessMultiset _pipelineAccesses;
  };
}
