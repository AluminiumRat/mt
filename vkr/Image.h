#pragma once

#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include <vkr/ImageAccess.h>
#include <vkr/RefCounter.h>

namespace mt
{
  class CommandQueue;
  class Device;

  //  Обертка вокруг VkImage
  //  Фиксированный кусок памяти, который используется как изображение
  //  (поддерживает чтение через сэмплеры и поддерживает лэйауты)
  class Image : public RefCounter
  {
  public:
    //  Создать пустой Image и выделить под него память на GPU
    //  VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE
    //  Память будет выделена по возможности в GPU-only куче, доступ со
    //    стороны CPU только через staging буферы
    Image(Device& device,
          VkImageType imageType,
          VkImageUsageFlags usageFlags,
          VkImageCreateFlags createFlags,
          VkFormat format,
          glm::uvec3 extent,
          VkSampleCountFlagBits samples,
          uint32_t arraySize,
          uint32_t mipmapCount,
          bool enableLayoutAutoControl);

    //  Создать объект-обертку вокруг уже существующего VkImage
    //  enableLayoutAutoControl - необходимо ли производить автоматический
    //    контроль лэйаута. Работает только если sharingMode равен
    //    VK_SHARING_MODE_EXCLUSIVE
    //  theLastAccess - последнее состояние лэйаута, в которое переведен Image
    //    после выполнения текущей очереди команд. Работает только для
    //    sharingMode VK_SHARING_MODE_EXCLUSIVE и включенного автоконтроля
    //    лэйаута. В остальных случаях следут передавать ImageAccess()
    Image(Device& device,
          VkImage handle,
          VkImageType imageType,
          VkFormat format,
          glm::uvec3 extent,
          VkSampleCountFlagBits samples,
          uint32_t arraySize,
          uint32_t mipmapCount,
          VkSharingMode sharingMode,
          bool enableLayoutAutoControl,
          const ImageAccess& theLastAccess);
    Image(const Image&) = delete;
    Image& operator = (const Image&) = delete;
  protected:
    virtual ~Image() noexcept;

  public:
    inline Device& device() const noexcept;

    inline VkImage handle() const noexcept;

    inline size_t memorySize() const noexcept;

    inline VkImageType imageType() const noexcept;
    inline VkFormat format() const noexcept;

    inline VkImageAspectFlags aspectMask() const noexcept;

    // Размер самого подробного мипа
    inline const glm::uvec3& extent() const noexcept;

    inline uint32_t mipmapCount() const noexcept;
    // Размер мипа
    inline glm::uvec3 extent(uint32_t mipLevel) const noexcept;

    inline VkSampleCountFlagBits samples() const noexcept;

    inline uint32_t arraySize() const noexcept;

    inline VkSharingMode sharingMode() const noexcept;
    inline bool isLayoutAutoControlEnabled() const noexcept;

    inline static uint32_t calculateMipNumber(const glm::uvec2& extent);

  private:
    void _cleanup() noexcept;

  private:
    //  Эти данные полностью контролируются классом CommandQueue и используются
    //  для автоматического преобразования лэйаутов. Это скорее ассоциированные
    //  с Image внешние данные, чем его внутреннее состояние.
    friend class CommandQueue;
    mutable CommandQueue* owner;
    mutable std::optional<ImageAccess> lastAccess;

  private:
    VkImage _handle;

    VmaAllocation _allocation;
    size_t _memorySize;

    VkImageType _imageType;
    VkFormat _format;

    VkImageAspectFlags _aspectMask;

    glm::uvec3 _extent;
    VkSampleCountFlagBits _samples;
    uint32_t _arraySize;
    uint32_t _mipmapCount;

    VkSharingMode _sharingMode;
    bool _layoutAutoControlEnabled;

    Device& _device;
  };

  inline Device& Image::device() const noexcept
  {
    return _device;
  }

  inline VkImage Image::handle() const noexcept
  {
    return _handle;
  }

  inline VkImageType Image::imageType() const noexcept
  {
    return _imageType;
  }

  inline VkFormat Image::format() const noexcept
  {
    return _format;
  }

  inline VkImageAspectFlags Image::aspectMask() const noexcept
  {
    return _aspectMask;
  }

  inline const glm::uvec3& Image::extent() const noexcept
  {
    return _extent;
  }

  inline glm::uvec3 Image::extent(uint32_t mipLevel) const noexcept
  {
    mipLevel = glm::min(mipLevel, _mipmapCount - 1);
    return glm::max(glm::uvec3( _extent.x >> mipLevel,
                                _extent.y >> mipLevel,
                                _extent.z >> mipLevel),
                    glm::uvec3(1));
  }

  inline VkSampleCountFlagBits Image::samples() const noexcept
  {
    return _samples;
  }

  inline uint32_t Image::arraySize() const noexcept
  {
    return _arraySize;
  }

  inline uint32_t Image::mipmapCount() const noexcept
  {
    return _mipmapCount;
  }

  inline size_t Image::memorySize() const noexcept
  {
    return _memorySize;
  }

  inline VkSharingMode Image::sharingMode() const noexcept
  {
    return _sharingMode;
  }

  inline bool Image::isLayoutAutoControlEnabled() const noexcept
  {
    return _layoutAutoControlEnabled;
  }

  inline uint32_t Image::calculateMipNumber(const glm::uvec2& extent)
  {
    unsigned int maxDimension = std::max(extent.x, extent.y);
    return uint32_t(std::floor(std::log2(maxDimension)) + 1);
  }
}