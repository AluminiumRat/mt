#include <glm/glm.hpp>

#include <util/Assert.h>
#include <vkr/Device.h>
#include <vkr/Image.h>

using namespace mt;

Image::Image( VkImage handle,
              VkImageType imageType,
              VkFormat format,
              glm::uvec3 extent,
              VkSampleCountFlagBits samples,
              uint32_t arraySize,
              uint32_t mipmapCount,
              VkSharingMode sharingMode,
              CommandQueue* owner,
              bool enableLayoutAutoControl,
              VkImageLayout lastLayoutInQueue,
              Device& device) :
  _handle(handle),
  _allocation(VK_NULL_HANDLE),
  _memorySize(0),
  _imageType(imageType),
  _format(format),
  _extent(extent),
  _samples(samples),
  _arraySize(arraySize),
  _mipmapCount(mipmapCount),
  _sharingMode(sharingMode),
  _owner(owner),
  _layoutAutoControlEnabled(enableLayoutAutoControl),
  _lastLayoutInQueue(lastLayoutInQueue),
  _device(device)
{
  MT_ASSERT(_handle != VK_NULL_HANDLE)
  MT_ASSERT(extent.x > 0 && extent.y > 0 && extent.z > 0)
  MT_ASSERT(arraySize > 0)
  MT_ASSERT(mipmapCount > 0)

  // Нельзя включать автоконтроль лэйаута, если image обрабатывается
  // несколькими очередями
  MT_ASSERT(!(enableLayoutAutoControl && (_sharingMode == VK_SHARING_MODE_EXCLUSIVE)));

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements( _device.handle(),
                                _handle,
                                &memoryRequirements);
  _memorySize = memoryRequirements.size;
}

Image::~Image()
{
  _cleanup();
}

void Image::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE && _allocation != VK_NULL_HANDLE)
  {
    vmaDestroyImage(_device.allocator(), _handle, _allocation);
    _handle = VK_NULL_HANDLE;
    _allocation = VK_NULL_HANDLE;
  }
}
