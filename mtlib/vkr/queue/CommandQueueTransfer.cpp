#include <vkr/image/Image.h>
#include <vkr/queue/CommandQueueTransfer.h>

using namespace mt;

CommandQueueTransfer::CommandQueueTransfer( Device& device,
                                            uint32_t familyIndex,
                                            uint32_t queueIndex,
                                            const QueueFamily& family,
                                            std::recursive_mutex& commonMutex) :
  CommandQueue(device, familyIndex, queueIndex, family, commonMutex)
{
}

std::unique_ptr<CommandProducerTransfer> CommandQueueTransfer::startCommands(
                                                  const char* producerDebugName)
{
  std::lock_guard lock(commonMutex());
  return std::make_unique<CommandProducerTransfer>( commonPoolSet(),
                                                    producerDebugName);
}

void CommandQueueTransfer::uploadToBuffer(const DataBuffer& dstBuffer,
                                          size_t shiftInDstBuffer,
                                          size_t dataSize,
                                          const void* srcData)
{
  std::unique_ptr<CommandProducerTransfer> producer = startCommands();
  producer->uploadToBuffer(dstBuffer, shiftInDstBuffer, dataSize, srcData);
  submitCommands(std::move(producer));
}

void CommandQueueTransfer::uploadToImage( const Image& dstImage,
                                          VkImageAspectFlags dstAspectMask,
                                          uint32_t dstArrayIndex,
                                          uint32_t dstMipLevel,
                                          glm::uvec3 dstOffset,
                                          glm::uvec3 dstExtent,
                                          const void* srcData)
{
  std::unique_ptr<CommandProducerTransfer> producer = startCommands();
  producer->uploadToImage(dstImage,
                          dstAspectMask,
                          dstArrayIndex,
                          dstMipLevel,
                          dstOffset,
                          dstExtent,
                          srcData);
  submitCommands(std::move(producer));
}