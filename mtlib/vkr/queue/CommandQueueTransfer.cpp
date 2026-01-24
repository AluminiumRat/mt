#include <vkr/image/Image.h>
#include <vkr/image/ImageFormatFeatures.h>
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
                                          const void* srcData)
{
  std::unique_ptr<CommandProducerTransfer> producer = startCommands();

  if(dataSize <= CommandPool::memoryPoolChunkSize)
  {
    //  Данные можно протолкнуть через UniformMemoryPool
    UniformMemoryPool::MemoryInfo stagingInfo =
                          producer->uniformMemorySession().write(
                                                          (const char*)srcData,
                                                          dataSize);
    producer->copyFromBufferToBuffer( *stagingInfo.buffer,
                                      dstBuffer,
                                      stagingInfo.offset,
                                      shiftInDstBuffer,
                                      dataSize);
  }
  else
  {
    //  Кусок данных слишком большой, протаскиваем его через отдельный
    //  upload буфер
    Ref<DataBuffer> stagingBuffer(new DataBuffer( device(),
                                                  dataSize,
                                                  DataBuffer::UPLOADING_BUFFER,
                                                  "Uploading buffer"));
    stagingBuffer->uploadData(srcData, 0, dataSize);
    producer->copyFromBufferToBuffer( *stagingBuffer,
                                      dstBuffer,
                                      0,
                                      0,
                                      dataSize);
  }
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
  uint32_t texelSize = getFormatFeatures(dstImage.format()).texelSize;
  size_t dataSize = size_t(texelSize) * dstExtent.x * dstExtent.y * dstExtent.z;
  dataSize = dataSize / 8 + (dataSize % 8 == 0 ? 0 : 1);
  Ref<DataBuffer> stagingBuffer(new DataBuffer( device(),
                                                dataSize,
                                                DataBuffer::UPLOADING_BUFFER,
                                                "Uploading buffer"));
  stagingBuffer->uploadData(srcData, 0, dataSize);

  std::unique_ptr<CommandProducerTransfer> producer = startCommands();

  producer->copyFromBufferToImage(*stagingBuffer,
                                  0,
                                  dstExtent.x,
                                  dstExtent.y,
                                  dstImage,
                                  dstAspectMask,
                                  dstArrayIndex,
                                  1,
                                  dstMipLevel,
                                  dstOffset,
                                  dstExtent);

  submitCommands(std::move(producer));
}