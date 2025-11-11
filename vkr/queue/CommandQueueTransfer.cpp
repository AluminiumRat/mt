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

std::unique_ptr<CommandProducerTransfer> CommandQueueTransfer::startCommands()
{
  std::lock_guard lock(commonMutex);
  return std::make_unique<CommandProducerTransfer>(commonPoolSet);
}

void CommandQueueTransfer::uploadToBuffer(const DataBuffer& dstBuffer,
                                          size_t shiftInDstBuffer,
                                          size_t dataSize,
                                          void* srcData)
{
  Ref<DataBuffer> stagingBuffer(new DataBuffer( device(),
                                                dataSize,
                                                DataBuffer::UPLOADING_BUFFER));
  stagingBuffer->uploadData(srcData, 0, dataSize);
  std::unique_ptr<CommandProducerTransfer> producer = startCommands();
  producer->copyFromBufferToBuffer( *stagingBuffer,
                                    dstBuffer,
                                    0,
                                    0,
                                    dataSize);
  submitCommands(std::move(producer));
}