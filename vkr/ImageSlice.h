#pragma once

#include <vulkan/vulkan.h>

#include <util/Assert.h>

namespace mt
{
  class Image;

  //  Некоторая часть Image-а, которая может рассматриваться как отдельный
  //    объект. В частности, может иметь собственный layout или служить
  //    источником данных для ImageView.
  //  Обратите внимание, ImageSlice не использует VK_REMAINING_MIP_LEVELS и
  //    VK_REMAINING_ARRAY_LAYERS для хранения данных, геттеры не возвращают
  //    никаких магических чисел
  class ImageSlice
  {
  public:
    // Пустой слайс
    inline ImageSlice() noexcept;
    // Создать слайс, который полностью закрывает весь Image
    explicit ImageSlice(const Image& image) noexcept;
    // Создать слайс, который закрывает только часть Image
    ImageSlice( const Image& image,
                VkImageAspectFlags aspectMask,
                uint32_t baseMipLevel,
                uint32_t levelCount = VK_REMAINING_MIP_LEVELS,
                uint32_t baseArrayLayer = 0,
                uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS) noexcept;
    ImageSlice(const ImageSlice&) noexcept = default;
    ImageSlice& operator = (const ImageSlice&) noexcept = default;
    ~ImageSlice() noexcept = default;

    inline VkImageAspectFlags aspectMask() const noexcept;
    inline uint32_t baseMipLevel() const noexcept;
    inline uint32_t lastMipLevel() const noexcept;
    inline uint32_t levelCount() const noexcept;
    inline uint32_t baseArrayLayer() const noexcept;
    inline uint32_t lastArrayLayer() const noexcept;
    inline uint32_t layerCount() const noexcept;

    // Проверить, закрывает ли слайс весь image
    bool isSliceFull(const Image& image) const noexcept;

    //  Проверить, что part полностью входит в этот слайс
    inline bool contains(const ImageSlice& part) const noexcept;

    //  Проверить пересекаются ли два слайса. То есть проверить, есть ли данные,
    //    которые входят и в первый и во второй слайт.
    inline bool isIntersected(const ImageSlice& other) const noexcept;

    inline VkImageSubresourceRange makeRange() const noexcept;

  private:
    VkImageAspectFlags _aspectMask;
    uint32_t _baseMipLevel;
    uint32_t _lastMipLevel;
    uint32_t _levelCount;
    uint32_t _baseArrayLayer;
    uint32_t _lastArrayLayer;
    uint32_t _layerCount;
  };

  inline bool operator == (
                  const ImageSlice& first, const ImageSlice& second) noexcept;
  inline bool operator != (
                  const ImageSlice& first, const ImageSlice& second) noexcept;

  inline ImageSlice::ImageSlice() noexcept :
    _aspectMask(0),
    _baseMipLevel(0),
    _lastMipLevel(0),
    _levelCount(1),
    _baseArrayLayer(0),
    _lastArrayLayer(0),
    _layerCount(1)
  {
  }

  inline VkImageAspectFlags ImageSlice::aspectMask() const noexcept
  {
    return _aspectMask;
  }

  inline uint32_t ImageSlice::baseMipLevel() const noexcept
  {
    return _baseMipLevel;
  }

  inline uint32_t ImageSlice::lastMipLevel() const noexcept
  {
    return _lastMipLevel;
  }

  inline uint32_t ImageSlice::levelCount() const noexcept
  {
    return _levelCount;
  }

  inline uint32_t ImageSlice::baseArrayLayer() const noexcept
  {
    return _baseArrayLayer;
  }

  inline uint32_t ImageSlice::lastArrayLayer() const noexcept
  {
    return _lastArrayLayer;
  }

  inline uint32_t ImageSlice::layerCount() const noexcept
  {
    return _layerCount;
  }

  inline bool operator == (
                  const ImageSlice& first, const ImageSlice& second) noexcept
  {
    return  first.aspectMask() == second.aspectMask() &&
            first.baseMipLevel() == second.baseMipLevel() &&
            first.levelCount() == second.levelCount() &&
            first.baseArrayLayer() == second.baseArrayLayer() &&
            first.layerCount() == second.layerCount();
  }

  inline bool operator != (
                  const ImageSlice& first, const ImageSlice& second) noexcept
  {
    return !(first == second);
  }

  inline bool ImageSlice::contains(const ImageSlice& part) const noexcept
  {
    if((_aspectMask & part._aspectMask) != part._aspectMask) return false;

    if(_baseMipLevel > part._baseMipLevel) return false;
    if(_lastMipLevel < part._lastMipLevel) return false;

    if (_baseArrayLayer > part._baseArrayLayer) return false;
    if (_lastArrayLayer < part._lastArrayLayer) return false;

    return true;
  }

  inline bool ImageSlice::isIntersected(const ImageSlice& other) const noexcept
  {
    if ((_aspectMask & other._aspectMask) == 0) return false;
    if (_lastMipLevel < other._baseMipLevel) return false;
    if (other._lastMipLevel < _baseMipLevel) return false;
    if (_lastArrayLayer < other._baseArrayLayer) return false;
    if (other._lastArrayLayer < _baseArrayLayer) return false;
    return true;
  }

  inline VkImageSubresourceRange ImageSlice::makeRange() const noexcept
  {
    return VkImageSubresourceRange{ .aspectMask = _aspectMask,
                                    .baseMipLevel = _baseMipLevel,
                                    .levelCount = _levelCount,
                                    .baseArrayLayer = _baseArrayLayer,
                                    .layerCount = _layerCount};
  }
}