#include <vkr/Image.h>
#include <vkr/ImageSlice.h>

using namespace mt;

ImageSlice::ImageSlice(const Image& image) noexcept :
  _aspectMask(image.aspectMask()),
  _baseMipLevel(0),
  _lastMipLevel(image.mipmapCount() - 1),
  _levelCount(image.mipmapCount()),
  _baseArrayLayer(0),
  _lastArrayLayer(image.arraySize() - 1),
  _layerCount(image.arraySize())
{
}

ImageSlice::ImageSlice( const Image& image,
                        VkImageAspectFlags aspectMask,
                        uint32_t baseMipLevel,
                        uint32_t baseArrayLayer,
                        uint32_t levelCount,
                        uint32_t layerCount) noexcept :
  _aspectMask(aspectMask),
  _baseMipLevel(baseMipLevel),
  _lastMipLevel(levelCount == VK_REMAINING_MIP_LEVELS ?
                                              image.mipmapCount() - 1 :
                                              _baseMipLevel + levelCount - 1),
  _levelCount(_lastMipLevel - _lastMipLevel + 1),
  _baseArrayLayer(baseArrayLayer),
  _lastArrayLayer(layerCount == VK_REMAINING_ARRAY_LAYERS ?
                                            image.arraySize() - 1 :
                                            _baseArrayLayer + layerCount - 1),
  _layerCount(_lastArrayLayer - _baseArrayLayer + 1)
{
  // aspectMask дожна быть строго подмножеством image.aspectMask
  MT_ASSERT((aspectMask & image.aspectMask()) != 0);
  MT_ASSERT((aspectMask & ~image.aspectMask()) == 0);

  MT_ASSERT(_baseMipLevel < image.mipmapCount());
  MT_ASSERT(_lastMipLevel < image.mipmapCount());

  MT_ASSERT(_baseArrayLayer < image.arraySize());
  MT_ASSERT(_lastArrayLayer < image.arraySize());
}

bool ImageSlice::isSliceFull(const Image& image) const noexcept
{
  return  _aspectMask == image.aspectMask() &&
          _baseMipLevel == 0 &&
          _levelCount == image.mipmapCount() &&
          _baseArrayLayer == 0 &&
          _layerCount == image.arraySize();
}