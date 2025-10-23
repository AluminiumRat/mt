#include <stdexcept>

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
  _level(level)
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
  if(_handle != VK_NULL_HANDLE)
  {
    vkFreeCommandBuffers( _device.handle(),
                          _pool,
                          1,
                          &_handle);
    _handle = VK_NULL_HANDLE;
  }
}

void CommandBuffer::imageBarrier( const ImageSlice& slice,
                                  VkImageLayout srcLayout,
                                  VkImageLayout dstLayout,
                                  VkPipelineStageFlags srcStages,
                                  VkPipelineStageFlags dstStages,
                                  VkAccessFlags srcAccesMask,
                                  VkAccessFlags dstAccesMask) noexcept
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