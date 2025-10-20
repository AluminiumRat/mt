#include <stdexcept>

#include <util/Assert.h>

#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/PlainBuffer.h>

using namespace mt;

PlainBuffer::Mapper::Mapper(PlainBuffer& buffer,
                            TransferDirection transferDirection) :
  _buffer(buffer),
  _data(nullptr),
  _transferDirection(transferDirection)
{
  if(vmaMapMemory(_buffer._device.allocator(),
                  _buffer._allocation,
                  &_data) != VK_SUCCESS)
  {
    throw std::runtime_error("PlainBuffer: Failed to map buffer's memory.");
  }

  if (_transferDirection == GPU_TO_CPU ||
      _transferDirection == BIDIRECTIONAL)
  {
    if(vmaInvalidateAllocation( _buffer._device.allocator(),
                                _buffer._allocation,
                                0,
                                _buffer.size()) != VK_SUCCESS)
    {
      throw std::runtime_error("PlainBuffer: Failed to vmaInvalidateAllocation");
    }
  }
}

PlainBuffer::Mapper::~Mapper()
{
  if(_data != nullptr)
  {
    if (_transferDirection == CPU_TO_GPU ||
        _transferDirection == BIDIRECTIONAL)
    {
      if(vmaFlushAllocation(_buffer._device.allocator(),
                            _buffer._allocation,
                            0,
                            _buffer.size()) != VK_SUCCESS)
      {
        MT_ASSERT(false && "PlainBuffer: Failed to vmaFlushAllocation");
      }
    }

    vmaUnmapMemory(_buffer._device.allocator(), _buffer._allocation);
  }
}

static VkBufferUsageFlags getUsageFlags(PlainBuffer::Usage usage)
{
  switch (usage)
  {
    case PlainBuffer::UPLOAD_BUFFER:
      return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    case PlainBuffer::DOWNLOAD_BUFFER:
      return VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case PlainBuffer::INDICES_BUFFER:
      return  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case PlainBuffer::UNIFORM_BUFFER:
      return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case PlainBuffer::VOLATILE_UNIFORM_BUFFER:
      return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }

  MT_ASSERT(false && "PlainBuffer::getUsageFlags: unknown usage");
};

static VmaAllocationCreateInfo getVmaCreateInfo(PlainBuffer::Usage usage)
{
  VmaAllocationCreateInfo info{};

  switch (usage)
  {
    case PlainBuffer::UPLOAD_BUFFER:
    case PlainBuffer::VOLATILE_UNIFORM_BUFFER:
      info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      break;

    case PlainBuffer::DOWNLOAD_BUFFER:
      info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      info.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
      break;

    case PlainBuffer::INDICES_BUFFER:
    case PlainBuffer::UNIFORM_BUFFER:
      info.requiredFlags = 0;
      info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      break;

    default:
      MT_ASSERT(false && "PlainBuffer::getVmaCreateInfo: unknown usage");
  }

  return info;
}

PlainBuffer::PlainBuffer( Device& device,
                          size_t size,
                          Usage usage) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _size(size),
  _allocation(VK_NULL_HANDLE)
{
  try
  {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = getUsageFlags(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo = getVmaCreateInfo(usage);

    if(vmaCreateBuffer( _device.allocator(),
                        &bufferInfo,
                        &allocCreateInfo,
                        &_handle,
                        &_allocation,
                        nullptr) != VK_SUCCESS)
    {
      throw std::runtime_error("PlainBuffer: Failed to create buffer.");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

PlainBuffer::PlainBuffer( Device& device,
                          size_t size,
                          VkBufferUsageFlags bufferUsageFlags,
                          VkMemoryPropertyFlags requiredFlags,
                          VkMemoryPropertyFlags preferredFlags) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _size(size),
  _allocation(VK_NULL_HANDLE)
{
  try
  {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = bufferUsageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.requiredFlags = requiredFlags;
    allocCreateInfo.preferredFlags = preferredFlags;

    if(vmaCreateBuffer( _device.allocator(),
                        &bufferInfo,
                        &allocCreateInfo,
                        &_handle,
                        &_allocation,
                        nullptr) != VK_SUCCESS)
    {
      throw std::runtime_error("PlainBuffer: Failed to create buffer.");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

PlainBuffer::~PlainBuffer()
{
  _cleanup();
}

void PlainBuffer::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vmaDestroyBuffer(_device.allocator(), _handle, _allocation);
    _allocation = VK_NULL_HANDLE;
    _handle = VK_NULL_HANDLE;
  }
}

void PlainBuffer::uploadData(const void* data, size_t shift, size_t dataSize)
{
  void* bufferData;
  if(vmaMapMemory(_device.allocator(),
                  _allocation,
                  &bufferData) != VK_SUCCESS)
  {
    throw std::runtime_error("PlainBuffer: Failed to map buffer's memory.");
  }

  memcpy((char*)bufferData + shift, data, dataSize);

  if(vmaFlushAllocation(_device.allocator(),
                        _allocation,
                        shift,
                        dataSize) != VK_SUCCESS)
  {
    MT_ASSERT(false && "PlainBuffer: Failed to vmaFlushAllocation");
  }

  vmaUnmapMemory(_device.allocator(), _allocation);
}
