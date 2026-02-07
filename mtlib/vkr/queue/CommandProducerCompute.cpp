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
  MT_ASSERT(descriptorSet.isFinalized());

  lockResource(layout);
  lockResource(descriptorSet);
  addMultipleImagesUsage(descriptorSet.imagesAccess().accessTable());

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
}

void CommandProducerCompute::dispatch(uint32_t gridSizeX,
                                      uint32_t gridSizeY,
                                      uint32_t gridSizeZ)
{
  MT_ASSERT(!insideRenderPass());

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdDispatch(buffer.handle(), gridSizeX, gridSizeY, gridSizeZ);
}
