#include <stdexcept>

#include <util/Assert.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/Device.h>
#include <vkr/Image.h>
#include <vkr/ImageSlice.h>

using namespace mt;

CommandBuffer::CommandBuffer( VkCommandPool pool,
                              Device& device,
                              VkCommandBufferLevel level) :
  _handle(VK_NULL_HANDLE),
  _pool(pool),
  _device(device),
  _level(level),
  _bufferInProcess(false)
{
  try
  {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers( _device.handle(),
                                  &allocInfo,
                                  &_handle) != VK_SUCCESS)
    {
      throw std::runtime_error("CommandBuffer: Failed to allocate command buffer.");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  };
}

CommandBuffer::~CommandBuffer()
{
  _cleanup();
}

void CommandBuffer::_cleanup() noexcept
{
  endBuffer();

  if(_handle != VK_NULL_HANDLE)
  {
    vkFreeCommandBuffers( _device.handle(),
                          _pool,
                          1,
                          &_handle);
    _handle = VK_NULL_HANDLE;
  }
}

void CommandBuffer::startOnetimeBuffer() noexcept
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(_handle, &beginInfo) != VK_SUCCESS)
  {
    MT_ASSERT(false && "CommandProducer: Failed to begin recording command buffer.");
  }

  _bufferInProcess = true;
}

void CommandBuffer::endBuffer() noexcept
{
  if(_bufferInProcess)
  {
    if (vkEndCommandBuffer(_handle) != VK_SUCCESS)
    {
      MT_ASSERT(false && "CommandProducer: Failed to end command buffer");
    }
    _bufferInProcess = false;
  }
}

void CommandBuffer::pipelineBarrier(
                                VkPipelineStageFlags srcStages,
                                VkPipelineStageFlags dstStages) const noexcept
{
  vkCmdPipelineBarrier( _handle,
                        srcStages,
                        dstStages,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        0,
                        nullptr);
}

void CommandBuffer::memoryBarrier(VkPipelineStageFlags srcStages,
                                  VkPipelineStageFlags dstStages,
                                  VkAccessFlags srcAccesMask,
                                  VkAccessFlags dstAccesMask) const noexcept
{
  VkMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

  barrier.srcAccessMask = srcAccesMask;
  barrier.dstAccessMask = dstAccesMask;

  vkCmdPipelineBarrier( _handle,
                        srcStages,
                        dstStages,
                        0,
                        1,
                        &barrier,
                        0,
                        nullptr,
                        0,
                        nullptr);
}

void CommandBuffer::imageBarrier( const ImageSlice& slice,
                                  VkImageLayout srcLayout,
                                  VkImageLayout dstLayout,
                                  VkPipelineStageFlags srcStages,
                                  VkPipelineStageFlags dstStages,
                                  VkAccessFlags srcAccesMask,
                                  VkAccessFlags dstAccesMask) const noexcept
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = srcLayout;
  barrier.newLayout = dstLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = slice.image().handle();
  barrier.subresourceRange = slice.makeRange();

  barrier.srcAccessMask = srcAccesMask;
  barrier.dstAccessMask = dstAccesMask;

  vkCmdPipelineBarrier( _handle,
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
