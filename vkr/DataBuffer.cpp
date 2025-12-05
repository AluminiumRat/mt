#include <stdexcept>

#include <util/Assert.h>

#include <vkr/DataBuffer.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>

using namespace mt;

DataBuffer::Mapper::Mapper( DataBuffer& buffer,
                            TransferDirection transferDirection) :
  _buffer(&buffer),
  _data(nullptr),
  _transferDirection(transferDirection)
{
  if(vmaMapMemory(_buffer->_device.allocator(),
                  _buffer->_allocation,
                  &_data) != VK_SUCCESS)
  {
    throw std::runtime_error("DataBuffer: Failed to map buffer's memory.");
  }

  if (_transferDirection == GPU_TO_CPU ||
      _transferDirection == BIDIRECTIONAL)
  {
    if(vmaInvalidateAllocation( _buffer->_device.allocator(),
                                _buffer->_allocation,
                                0,
                                _buffer->size()) != VK_SUCCESS)
    {
      throw std::runtime_error("DataBuffer: Failed to vmaInvalidateAllocation");
    }
  }
}

void DataBuffer::Mapper::_unmap() noexcept
{
  if(_data != nullptr)
  {
    if (_transferDirection == CPU_TO_GPU ||
        _transferDirection == BIDIRECTIONAL)
    {
      if(vmaFlushAllocation(_buffer->_device.allocator(),
                            _buffer->_allocation,
                            0,
                            _buffer->size()) != VK_SUCCESS)
      {
        MT_ASSERT(false && "DataBuffer: Failed to vmaFlushAllocation");
      }
    }

    vmaUnmapMemory(_buffer->_device.allocator(), _buffer->_allocation);

    _data = nullptr;
    _buffer = nullptr;
  }
}

DataBuffer::Mapper::~Mapper()
{
  _unmap();
}

static VkBufferUsageFlags getUsageFlags(DataBuffer::Usage usage)
{
  switch (usage)
  {
    case DataBuffer::UPLOADING_BUFFER:
      return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    case DataBuffer::DOWNLOADING_BUFFER:
      return VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case DataBuffer::INDICES_BUFFER:
      return  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case DataBuffer::UNIFORM_BUFFER:
      return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    case DataBuffer::VOLATILE_UNIFORM_BUFFER:
      return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    case DataBuffer::STORAGE_BUFFER:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  Abort("DataBuffer::getUsageFlags: unknown usage");
};

static VmaAllocationCreateInfo getVmaCreateInfo(DataBuffer::Usage usage)
{
  VmaAllocationCreateInfo info{};

  switch (usage)
  {
    case DataBuffer::UPLOADING_BUFFER:
    case DataBuffer::VOLATILE_UNIFORM_BUFFER:
      info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      break;

    case DataBuffer::DOWNLOADING_BUFFER:
      info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      info.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
      break;

    case DataBuffer::INDICES_BUFFER:
    case DataBuffer::UNIFORM_BUFFER:
    case DataBuffer::STORAGE_BUFFER:
      info.requiredFlags = 0;
      info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      break;

    default:
      MT_ASSERT(false && "DataBuffer::getVmaCreateInfo: unknown usage");
  }

  return info;
}

DataBuffer::DataBuffer( Device& device,
                        size_t size,
                        Usage usage,
                        const char* debugName) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _size(size),
  _allocation(VK_NULL_HANDLE),
  _debugName(debugName)
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
      throw std::runtime_error("DataBuffer: Failed to create buffer " + _debugName);
    }

    _setDebugName();
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

DataBuffer::DataBuffer( Device& device,
                        size_t size,
                        VkBufferUsageFlags bufferUsageFlags,
                        VkMemoryPropertyFlags requiredFlags,
                        VkMemoryPropertyFlags preferredFlags,
                        const char* debugName) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _size(size),
  _allocation(VK_NULL_HANDLE),
  _debugName(debugName)
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
      throw std::runtime_error("DataBuffer: Failed to create buffer.");
    }

    _setDebugName();
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

void DataBuffer::_setDebugName() noexcept
{
  if(!VKRLib::instance().isDebugEnabled()) return;

  VkDebugUtilsObjectNameInfoEXT nameInfo{};
  nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
  nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
  nameInfo.objectHandle = (uint64_t)_handle;
  nameInfo.pObjectName = _debugName.c_str();

  _device.extFunctions().vkSetDebugUtilsObjectNameEXT(&nameInfo);
}

DataBuffer::~DataBuffer()
{
  _cleanup();
}

void DataBuffer::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vmaDestroyBuffer(_device.allocator(), _handle, _allocation);
    _allocation = VK_NULL_HANDLE;
    _handle = VK_NULL_HANDLE;
  }
}

void DataBuffer::uploadData(const void* data, size_t shift, size_t dataSize)
{
  void* bufferData;
  if(vmaMapMemory(_device.allocator(),
                  _allocation,
                  &bufferData) != VK_SUCCESS)
  {
    throw std::runtime_error("DataBuffer: Failed to map buffer's memory.");
  }

  memcpy((char*)bufferData + shift, data, dataSize);

  if(vmaFlushAllocation(_device.allocator(),
                        _allocation,
                        shift,
                        dataSize) != VK_SUCCESS)
  {
    MT_ASSERT(false && "DataBuffer: Failed to vmaFlushAllocation");
  }

  vmaUnmapMemory(_device.allocator(), _allocation);
}
