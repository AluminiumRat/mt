#pragma once

#include <cmath>
#include <vector>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vkr/RefCounter.h>

namespace mt
{
  class CommandQueue;
  class Device;

  // Обертка вокруг VkImage
  // Фиксированный кусок памяти, который используется как изображение
  // (поддерживает чтение через сэмплеры и поддерживает лэйауты)
  class Image : public RefCounter
  {
  public:
    //  Создать объект-обертку вокруг уже существующего Image
    //  owner - очередь, к которой привязан Image. Если ни одна из очередей
    //    ещё не захватила владение image-ем, то необходимо передать nullptr
    //    если sharingMode == VK_SHARING_MODE_EXCLUSIVE, то необходимо передать
    //    nullptr
    //  enableLayoutAutoControl - необходимо ли производить автоматический
    //    контроль лэйаута. Работает только если sharingMode равен
    //    VK_SHARING_MODE_EXCLUSIVE
    //  lastLayoutInQueue - тот лэйаут, в который будет переведен Image после
    //    выполнения текущей очереди команд. Работает только для sharingMode
    //    VK_SHARING_MODE_EXCLUSIVE и включенного автоконтроля лэйаута. В
    //    остальных случаях следут передавать VK_IMAGE_LAYOUT_UNDEFINED
    Image(VkImage handle,
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
          Device& device);
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
    
    // Размер самого подробного мипа
    inline const glm::uvec3& extent() const noexcept;

    inline uint32_t mipmapCount() const noexcept;
    // Размер мипа
    inline glm::uvec3 extent(uint32_t mipLevel) const noexcept;
    
    inline VkSampleCountFlagBits samples() const noexcept;

    inline uint32_t arraySize() const noexcept;

    inline VkSharingMode sharingMode() const noexcept;
    inline CommandQueue* owner() const noexcept;
    inline bool isLayoutAutoControlEnabled() const noexcept;
    inline VkImageLayout lastLayoutInQueue() const noexcept;

    inline static uint32_t calculateMipNumber(const glm::uvec2& extent);

  private:
    void _cleanup() noexcept;

  private:
    VkImage _handle;

    VmaAllocation _allocation;
    size_t _memorySize;

    VkImageType _imageType;
    VkFormat _format;

    glm::uvec3 _extent;
    VkSampleCountFlagBits _samples;
    uint32_t _arraySize;
    uint32_t _mipmapCount;

    VkSharingMode _sharingMode;
    CommandQueue* _owner;
    bool _layoutAutoControlEnabled;
    VkImageLayout _lastLayoutInQueue;

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

  inline CommandQueue* Image::owner() const noexcept
  {
    return _owner;
  }

  inline bool Image::isLayoutAutoControlEnabled() const noexcept
  {
    return _layoutAutoControlEnabled;
  }

  inline VkImageLayout Image::lastLayoutInQueue() const noexcept
  {
    return _lastLayoutInQueue;
  }

  inline uint32_t Image::calculateMipNumber(const glm::uvec2& extent)
  {
    unsigned int maxDimension = std::max(extent.x, extent.y);
    return uint32_t(std::floor(std::log2(maxDimension)) + 1);
  }
}