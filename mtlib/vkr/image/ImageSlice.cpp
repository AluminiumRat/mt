#include <vkr/image/Image.h>
#include <vkr/image/ImageSlice.h>

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
                        uint32_t levelCount,
                        uint32_t baseArrayLayer,
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

  _baseMipLevel = std::min(_baseMipLevel, image.mipmapCount() - 1);
  _lastMipLevel = std::min(_lastMipLevel, image.mipmapCount() - 1);

  _baseArrayLayer = std::min(_baseArrayLayer, image.arraySize() - 1);
  _lastArrayLayer = std::min(_lastArrayLayer, image.arraySize() - 1);
}

ImageSlice::ImageSlice( VkImageAspectFlags aspectMask,
                        uint32_t baseMipLevel,
                        uint32_t levelCount,
                        uint32_t baseArrayLayer,
                        uint32_t layerCount) noexcept :
  _aspectMask(aspectMask),
  _baseMipLevel(baseMipLevel),
  _lastMipLevel(_baseMipLevel + levelCount - 1),
  _levelCount(levelCount),
  _baseArrayLayer(baseArrayLayer),
  _lastArrayLayer( _baseArrayLayer + layerCount - 1),
  _layerCount(layerCount)
{
  MT_ASSERT(aspectMask != 0);
  MT_ASSERT(levelCount != 0);
  MT_ASSERT(layerCount != 0);
}

bool ImageSlice::isSliceFull(const Image& image) const noexcept
{
  return  _aspectMask == image.aspectMask() &&
          _baseMipLevel == 0 &&
          _levelCount == image.mipmapCount() &&
          _baseArrayLayer == 0 &&
          _layerCount == image.arraySize();
}