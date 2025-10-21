#include <stdexcept>

#include <vkr/queue/CommandBuffer.h>
#include <vkr/Device.h>

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
