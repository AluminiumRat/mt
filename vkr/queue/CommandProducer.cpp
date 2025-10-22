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

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _commandBuffer(nullptr),
  _bufferInProcess(false),
  _descriptorPool(nullptr)
{
}

void CommandProducer::finalize()
{
  if(_uniformMemorySession.has_value())
  {
    _uniformMemorySession.reset();
  }

  if(_commandBuffer != nullptr && _bufferInProcess)
  {
    _bufferInProcess = false;
    if (vkEndCommandBuffer(_commandBuffer->handle()) != VK_SUCCESS)
    {
      throw std::runtime_error("CommandProducer: Failed to end command buffer.");
    }
  }
}

void CommandProducer::release(const SyncPoint& releasePoint)
{
  finalize();

  _descriptorPool = nullptr;
  _commandBuffer = nullptr;

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
  if(_commandBuffer != nullptr) return *_commandBuffer;

  if(_commandPool == nullptr) _commandPool = &_commandPoolSet.getPool();
  if(_descriptorPool == nullptr)
  {
    _descriptorPool = &_commandPool->descriptorPool();
  }

  _uniformMemorySession.emplace(_commandPool->memoryPool());
  _commandBuffer = &_commandPool->getNextBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(_commandBuffer->handle(), &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandProducer: Failed to begin recording command buffer.");
  }
  _bufferInProcess = true;

  return *_commandBuffer;
}

void CommandProducer::imageBarrier( Image& image,
                                    VkImageLayout srcLayout,
                                    VkImageLayout dstLayout,
                                    VkImageAspectFlags aspectMask,
                                    uint32_t baseMip,
                                    uint32_t mipCount,
                                    uint32_t baseArrayLayer,
                                    uint32_t layerCount,
                                    VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  MT_ASSERT(!image.isLayoutAutoControlEnabled());

  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.lockResource(image);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = srcLayout;
  barrier.newLayout = dstLayout;
  if(image.sharingMode() == VK_SHARING_MODE_EXCLUSIVE)
  {
    barrier.srcQueueFamilyIndex = _queue.family().index();
    barrier.dstQueueFamilyIndex = _queue.family().index();
  }
  else
  {
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  }
  barrier.image = image.handle();
  barrier.subresourceRange.aspectMask = aspectMask;
  barrier.subresourceRange.baseMipLevel = baseMip;
  barrier.subresourceRange.levelCount = mipCount;
  barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
  barrier.subresourceRange.layerCount = layerCount;

  barrier.srcAccessMask = srcAccesMask;
  barrier.dstAccessMask = dstAccesMask;

  vkCmdPipelineBarrier( buffer.handle(),
                        srcStages,
                        dstStages,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier);
}
