#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandPool.h>
#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandProducer.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/VolatileDescriptorPool.h>
#include <vkr/Image.h>
#include <vkr/ImageSlice.h>

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _currentPrimaryBuffer(nullptr),
  _cachedMatchingBuffer(nullptr),
  _descriptorPool(nullptr),
  _isFinalized(false)
{
  try
  {
    _commandPool = &_commandPoolSet.getPool();
    _descriptorPool = &_commandPool->descriptorPool();
    _uniformMemorySession.emplace(_commandPool->memoryPool());
    _commandSequence.reserve(1024);
  }
  catch (std::exception& error)
  {
    Log::error() << "CommandProducer: " << error.what();
    MT_ASSERT(false && "Unable to create CommandProducer");
  }
}

std::optional<CommandProducer::FinalizeResult>
                                          CommandProducer::finalize() noexcept
{
  MT_ASSERT(!_isFinalized);

  try
  {
    _isFinalized = true;

    _uniformMemorySession.reset();

    if(_commandSequence.empty()) return std::nullopt;

    FinalizeResult result;
    result.commandSequence = &_commandSequence;
    result.matchingBuffer = _cachedMatchingBuffer != nullptr ?
                                                _cachedMatchingBuffer :
                                                &_commandPool->getNextBuffer();
    result.imageStates = &_accessWatcher.finalize();

    //  EndBuffer стоит после _accessWatcher.finalize, так как finalize
    //  ещё может писать в буферы команд(согласование предпоследнего и
    //  последнего доступов)
    for(CommandBuffer* buffer : _commandSequence)
    {
      buffer->endBuffer();
    }

    return result;
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandProducer::finalize: " << error.what();
    Abort("Unable to finalize CommandProducer");
  }
}

void CommandProducer::release(const SyncPoint& releasePoint)
{
  if(!_isFinalized) finalize();

  if(_commandPool != nullptr)
  {
    _commandPoolSet.returnPool(*_commandPool, releasePoint);
    _commandPool = nullptr;
  }
}

CommandProducer::~CommandProducer() noexcept
{
  try
  {
    if(_commandPool != nullptr)
    {
      Log::warning() << "CommandProducer::~CommandProducer(): producer is not released.";
      release(_queue.createSyncPoint());
    }
  }
  catch (std::runtime_error& exception)
  {
    Log::error() << "CommandProducer::~CommandProducer: " << exception.what();
    MT_ASSERT(false && "Unable to close CommandProducer");
  }
}

CommandBuffer& CommandProducer::_getOrCreateBuffer()
{
  // Нельзя создавать новый буфер после финализации
  MT_ASSERT(!_isFinalized);

  if(_currentPrimaryBuffer != nullptr) return *_currentPrimaryBuffer;

  _currentPrimaryBuffer = &_commandPool->getNextBuffer();
  _currentPrimaryBuffer->startOnetimeBuffer();
  _commandSequence.push_back(_currentPrimaryBuffer);

  return *_currentPrimaryBuffer;
}

void CommandProducer::_addImageUsage( const Image& image,
                                      const ImageAccess& access)
{
  if(image.isLayoutAutoControlEnabled())
  {
    if(_cachedMatchingBuffer == nullptr)
    {
      _cachedMatchingBuffer = &_commandPool->getNextBuffer();
    }

    if(_accessWatcher.addImageAccess( image,
                                      access,
                                      *_cachedMatchingBuffer) ==
                                          ImageAccessWatcher::NEED_TO_MATCHING)
    {
      // Обнаружили место, где будет барьер
      CommandBuffer* matchingBuffer = _cachedMatchingBuffer;
      _cachedMatchingBuffer = nullptr;
      _currentPrimaryBuffer = nullptr;
      matchingBuffer->startOnetimeBuffer();
      _commandSequence.push_back(matchingBuffer);
    }
  }

  // Захватываем владение, чтобы Image не удалился во время выполнения буфера
  // Захватываем уже новым буфером команд после барьера
  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.lockResource(image);
}

void CommandProducer::_addBufferUsage(const PlainBuffer& buffer)
{
  CommandBuffer& commandbuffer = _getOrCreateBuffer();
  commandbuffer.lockResource(buffer);
}

void CommandProducer::memoryBarrier(VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.memoryBarrier(srcStages, dstStages, srcAccesMask, dstAccesMask);
}

void CommandProducer::imageBarrier( const Image& image,
                                    const ImageSlice& slice,
                                    VkImageLayout srcLayout,
                                    VkImageLayout dstLayout,
                                    VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  MT_ASSERT(!image.isLayoutAutoControlEnabled());

  ImageAccess imageAccess;
  imageAccess.slices[0] = slice;
  imageAccess.layouts[0] = dstLayout;
  imageAccess.memoryAccess[0] = MemoryAccess{
                                  .readStagesMask = dstStages,
                                  .readAccessMask = dstAccesMask,
                                  .writeStagesMask = dstStages,
                                  .writeAccessMask = dstAccesMask};

  _addImageUsage(image, imageAccess);

  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.imageBarrier(image,
                      slice,
                      srcLayout,
                      dstLayout,
                      srcStages,
                      dstStages,
                      srcAccesMask,
                      dstAccesMask);
}

void CommandProducer::forceLayout(const Image& image,
                                  const ImageSlice& slice,
                                  VkImageLayout dstLayout,
                                  VkPipelineStageFlags readStages,
                                  VkAccessFlags readAccessMask,
                                  VkPipelineStageFlags writeStages,
                                  VkAccessFlags writeAccessMask)
{
  MT_ASSERT(image.isLayoutAutoControlEnabled());

  ImageAccess imageAccess;
  imageAccess.slices[0] = slice;
  imageAccess.layouts[0] = dstLayout;
  imageAccess.memoryAccess[0] = MemoryAccess{
                                  .readStagesMask = readStages,
                                  .readAccessMask = readAccessMask,
                                  .writeStagesMask = writeStages,
                                  .writeAccessMask = writeAccessMask};
  imageAccess.slicesCount = 1;

  _addImageUsage( image, imageAccess);
}

void CommandProducer::copyFromBufferToImage(const PlainBuffer& srcBuffer,
                                            VkDeviceSize srcBufferOffset,
                                            uint32_t srcRowLength,
                                            uint32_t srcImageHeight,
                                            const Image& dstImage,
                                            VkImageAspectFlags dstAspectMask,
                                            uint32_t dstArrayIndex,
                                            uint32_t dstMipLevel,
                                            glm::uvec3 dstOffset,
                                            glm::uvec3 dstExtent)
{
  ImageAccess imageAccess;
  imageAccess.slices[0] = ImageSlice( dstImage,
                                      dstAspectMask,
                                      dstMipLevel,
                                      1,
                                      dstArrayIndex,
                                      1);
  imageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = 0,
                              .readAccessMask = 0,
                              .writeStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .writeAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT};
  imageAccess.slicesCount = 1;
  _addImageUsage(dstImage, imageAccess);
  _addBufferUsage(srcBuffer);

  VkBufferImageCopy region{};
  region.bufferOffset = srcBufferOffset;
  region.bufferRowLength = srcRowLength;
  region.bufferImageHeight = srcImageHeight;

  region.imageSubresource.aspectMask = dstAspectMask;
  region.imageSubresource.mipLevel = dstMipLevel;
  region.imageSubresource.baseArrayLayer = dstArrayIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset.x = uint32_t(dstOffset.x);
  region.imageOffset.y = uint32_t(dstOffset.y);
  region.imageOffset.z = uint32_t(dstOffset.z);

  region.imageExtent.width = uint32_t(dstExtent.x);
  region.imageExtent.height = uint32_t(dstExtent.y);
  region.imageExtent.depth = uint32_t(dstExtent.z);

  CommandBuffer& buffer = _getOrCreateBuffer();

  vkCmdCopyBufferToImage( buffer.handle(),
                          srcBuffer.handle(),
                          dstImage.handle(),
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1,
                          &region);
}

void CommandProducer::copyFromImageToBuffer(const Image& srcImage,
                                            VkImageAspectFlags srcAspectMask,
                                            uint32_t srcArrayIndex,
                                            uint32_t srcMipLevel,
                                            glm::uvec3 srcOffset,
                                            glm::uvec3 srcExtent,
                                            const PlainBuffer& dstBuffer,
                                            VkDeviceSize dstBufferOffset,
                                            uint32_t dstRowLength,
                                            uint32_t dstImageHeight)
{
  ImageAccess imageAccess;
  imageAccess.slices[0] = ImageSlice( srcImage,
                                      srcAspectMask,
                                      srcMipLevel,
                                      1,
                                      srcArrayIndex,
                                      1);
  imageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .readAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                              .writeStagesMask = 0,
                              .writeAccessMask = 0};
  imageAccess.slicesCount = 1;
  _addImageUsage(srcImage, imageAccess);
  _addBufferUsage(dstBuffer);

  VkBufferImageCopy region{};
  region.bufferOffset = dstBufferOffset;
  region.bufferRowLength = dstRowLength;
  region.bufferImageHeight = dstImageHeight;

  region.imageSubresource.aspectMask = srcAspectMask;
  region.imageSubresource.mipLevel = srcMipLevel;
  region.imageSubresource.baseArrayLayer = srcArrayIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset.x = uint32_t(srcOffset.x);
  region.imageOffset.y = uint32_t(srcOffset.y);
  region.imageOffset.z = uint32_t(srcOffset.z);

  region.imageExtent.width = uint32_t(srcExtent.x);
  region.imageExtent.height = uint32_t(srcExtent.y);
  region.imageExtent.depth = uint32_t(srcExtent.z);

  CommandBuffer& buffer = _getOrCreateBuffer();
  vkCmdCopyImageToBuffer( buffer.handle(),
                          srcImage.handle(),
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          dstBuffer.handle(),
                          1,
                          &region);
}

void CommandProducer::halfOwnershipTransfer(const Image& image,
                                            uint32_t oldFamilyIndex,
                                            uint32_t newFamilyIndex)
{
  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.lockResource(image);

  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.srcQueueFamilyIndex = oldFamilyIndex;
  imageBarrier.dstQueueFamilyIndex = newFamilyIndex;
  imageBarrier.image = image.handle();
  imageBarrier.subresourceRange = ImageSlice(image).makeRange();
  imageBarrier.srcAccessMask = 0;
  imageBarrier.dstAccessMask = 0;

  vkCmdPipelineBarrier( buffer.handle(),
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        1,
                        &memoryBarrier,
                        0,
                        nullptr,
                        1,
                        &imageBarrier);
}

void CommandProducer::halfOwnershipTransfer(const PlainBuffer& buffer,
                                            uint32_t oldFamilyIndex,
                                            uint32_t newFamilyIndex)
{
  CommandBuffer& commandBuffer = _getOrCreateBuffer();
  commandBuffer.lockResource(buffer);

  VkBufferMemoryBarrier bufferBarrier{};
  bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.srcQueueFamilyIndex = oldFamilyIndex;
  bufferBarrier.dstQueueFamilyIndex = newFamilyIndex;
  bufferBarrier.buffer = buffer.handle();
  bufferBarrier.size = buffer.size();
  bufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  bufferBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  vkCmdPipelineBarrier( commandBuffer.handle(),
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        0,
                        nullptr,
                        1,
                        &bufferBarrier,
                        0,
                        nullptr);
}