#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;

  // Просто фиксированный кусок памяти на стороне GPU
  // Обертка вокруг VkBuffer.
  class PlainBuffer : public RefCounter
  {
  public:
    enum Usage
    {
      UPLOAD_BUFFER,
      DOWNLOAD_BUFFER,
      INDICES_BUFFER,
      UNIFORM_BUFFER,
      VOLATILE_UNIFORM_BUFFER
    };

    //  Маппинг буфера на ЦПУ память. используется для записи в/из ГПУ
    //  Для загрузки данных на GPU лучше использовать PlainBuffer::uploadData,
    //    особенно, если необходимо загрузить только чать данных.
    //  ВНИМАНИЕ! Буфер должен быть создан с Usage UPLOAD_BUFFER или 
    //    DOWNLOAD_BUFFER или с
    //    requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    //  ВНИМАНИЕ! Не используйте одновременно несколько мапперов на одном
    //    и том же буфере.
    //  ВНИМАНИЕ! Не используйте одновременно маппер и метод
    //    PlainBuffer::uploadData на одном и том же буфере
    class Mapper
    {
    public:
      // Направление, в котором будут перемещаться данные.
      enum TransferDirection
      {
        CPU_TO_GPU,
        GPU_TO_CPU,
        BIDIRECTIONAL
      };

    public:
      Mapper(PlainBuffer& buffer, TransferDirection transferDirection);
      Mapper(const Mapper&) = delete;
      Mapper& operator = (const Mapper&) = delete;
      inline Mapper(Mapper&& other) noexcept;
      inline Mapper& operator = (Mapper&& other) noexcept;
      ~Mapper();

      inline void* data() const;

    private:
      void _unmap() noexcept;

    private:
      PlainBuffer* _buffer;
      void* _data;
      TransferDirection _transferDirection;
    };

  public:
    // Упрощенный конструктор для типового использования
    PlainBuffer(Device& device, size_t size, Usage usage);
    // Полнофункциональный конструктор для кастома
    PlainBuffer(Device& device,
                size_t size,
                VkBufferUsageFlags bufferUsageFlags,
                VkMemoryPropertyFlags requiredFlags,
                VkMemoryPropertyFlags preferredFlags);
    PlainBuffer(const PlainBuffer&) = delete;
    PlainBuffer& operator = (const PlainBuffer&) = delete;
  protected:
    virtual ~PlainBuffer();

  public:
    inline VkBuffer handle() const;
    inline size_t size() const;

    //  Загрузка данных с хоста на ГПУ.
    //  ВНИМАНИЕ! Буфер должен быть создан с Usage UPLOAD_BUFFER или 
    //    DOWNLOAD_BUFFER или с
    //    requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    //  ВНИМАНИЕ! Не используйте одновременно маппер и метод
    //    PlainBuffer::uploadData на одном и том же буфере
    void uploadData(const void* data, size_t shift, size_t dataSize);

  private:
    void _cleanup() noexcept;

  private:
    Device& _device;
    VkBuffer _handle;
    size_t _size;

    VmaAllocation _allocation;
  };

  inline PlainBuffer::Mapper::Mapper(PlainBuffer::Mapper&& other) noexcept :
    _buffer(other._buffer),
    _data(other._data),
    _transferDirection(other._transferDirection)
  {
    other._buffer = nullptr;
    other._data = nullptr;
  }

  inline PlainBuffer::Mapper& PlainBuffer::Mapper::operator = (
                                          PlainBuffer::Mapper&& other) noexcept
  {
    _unmap();

    _buffer = other._buffer;
    _data = other._data;
    _transferDirection = other._transferDirection;

    other._buffer = nullptr;
    other._data = nullptr;

    return *this;
  }

  inline VkBuffer PlainBuffer::handle() const
  {
    return _handle;
  }

  inline size_t PlainBuffer::size() const
  {
    return _size;
  }

  inline void* PlainBuffer::Mapper::data() const
  {
    return _data;
  }
}