#include <glm/glm.hpp>

#include <util/Assert.h>
#include <vkr/Device.h>
#include <vkr/Image.h>

using namespace mt;

Image::Image( Device& device,
              VkImageType imageType,
              VkImageUsageFlags usageFlags,
              VkImageCreateFlags createFlags,
              VkFormat format,
              VkImageAspectFlags aspectMask,
              glm::uvec3 extent,
              VkSampleCountFlagBits samples,
              uint32_t arraySize,
              uint32_t mipmapCount,
              bool enableLayoutAutoControl) :
  owner(nullptr),
  _handle(VK_NULL_HANDLE),
  _allocation(VK_NULL_HANDLE),
  _memorySize(0),
  _imageType(imageType),
  _format(format),
  _aspectMask(aspectMask),
  _extent(extent),
  _samples(samples),
  _arraySize(arraySize),
  _mipmapCount(mipmapCount),
  _sharingMode(VK_SHARING_MODE_EXCLUSIVE),
  _layoutAutoControlEnabled(enableLayoutAutoControl),
  _device(device)
{
  MT_ASSERT(extent.x > 0 && extent.y > 0 && extent.z > 0);
  MT_ASSERT(arraySize > 0);
  MT_ASSERT(mipmapCount > 0);

  try
  {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = _imageType;
    imageInfo.extent.width = uint32_t(_extent.x);
    imageInfo.extent.height = uint32_t(_extent.y);
    imageInfo.extent.depth = uint32_t(_extent.z);;
    imageInfo.mipLevels = _mipmapCount;
    imageInfo.arrayLayers = _arraySize;
    imageInfo.format = _format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = _samples;
    imageInfo.flags = createFlags;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocationInfo allocInfo{};

    if(vmaCreateImage(_device.allocator(),
                      &imageInfo,
                      &allocCreateInfo,
                      &_handle,
                      &_allocation,
                      &allocInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Image: Failed to create image.");
    }

    _memorySize = allocInfo.size;
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

Image::Image( Device& device,
              VkImage handle,
              VkImageType imageType,
              VkFormat format,
              VkImageAspectFlags aspectMask,
              glm::uvec3 extent,
              VkSampleCountFlagBits samples,
              uint32_t arraySize,
              uint32_t mipmapCount,
              VkSharingMode sharingMode,
              CommandQueue* theOwner,
              bool enableLayoutAutoControl,
              const ImageAccess& theLastAccess) :
  owner(theOwner),
  lastAccess(theLastAccess),
  _handle(handle),
  _allocation(VK_NULL_HANDLE),
  _memorySize(0),
  _imageType(imageType),
  _format(format),
  _aspectMask(aspectMask),
  _extent(extent),
  _samples(samples),
  _arraySize(arraySize),
  _mipmapCount(mipmapCount),
  _sharingMode(sharingMode),
  _layoutAutoControlEnabled(enableLayoutAutoControl),
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
