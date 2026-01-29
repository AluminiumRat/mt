#include <util/Assert.h>
#include <vkr/pipeline/ComputePipeline.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/queue/CommandProducerCompute.h>

using namespace mt;

CommandProducerCompute::CommandProducerCompute( CommandPoolSet& poolSet,
                                                const char* debugName) :
  CommandProducerTransfer(poolSet, debugName)
{
}

void CommandProducerCompute::setComputePipeline(const ComputePipeline& pipeline)
{
  lockResource(pipeline);

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindPipeline(buffer.handle(),
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline.handle());
}

void CommandProducerCompute::bindDescriptorSetCompute(
                                            const DescriptorSet& descriptorSet,
                                            uint32_t setIndex,
                                            const PipelineLayout& layout)
{
  MT_ASSERT(setIndex < maxComputeDescriptorSetsNumber);
  MT_ASSERT(descriptorSet.isFinalized());

  lockResource(layout);
  lockResource(descriptorSet);

  _pipelineAccesses.setChild(&descriptorSet.imagesAccess(), setIndex);

  VkDescriptorSet setHandle = descriptorSet.handle();
  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindDescriptorSets(buffer.handle(),
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          layout.handle(),
                          setIndex,
                          1,
                          &setHandle,
                          0,
                          nullptr);
}

void CommandProducerCompute::unbindDescriptorSetCompute(
                                                    uint32_t setIndex) noexcept
{
  MT_ASSERT(setIndex < maxComputeDescriptorSetsNumber);
  _pipelineAccesses.setChild(nullptr, setIndex);
}

void CommandProducerCompute::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  addMultipleImagesUsage(_pipelineAccesses.getMergedSet().accessTable());
  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdDispatch(buffer.handle(), x, y, z);
}
