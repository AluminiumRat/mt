#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

namespace mt
{
  class Device;
  class Image;
  class ImageSlice;

  //  Отдельный буфер команд для GPU
  //  Обертка вокруг VkCommandBuffer
  class CommandBuffer : public RefCounter
  {
  public:
    CommandBuffer(VkCommandPool pool,
                  Device& device,
                  VkCommandBufferLevel level);
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator = (const CommandBuffer&) = delete;
  protected:
    virtual ~CommandBuffer();

  public:
    inline VkCommandBuffer handle() const;
    inline VkCommandBufferLevel level() const;

    //  Стартовать запись команд для однократного использования
    void startOnetimeBuffer() noexcept;
    //  Финализировать запись команд в буфер
    void endBuffer() noexcept;

    //  Буфер находится в состоянии записи команд
    inline bool isBufferInProcess() const noexcept;

    //  Чистый пайплайн барьер, то есть просто разделяем поток исполнения
    //    команд без сброса кэшей
    void pipelineBarrier( VkPipelineStageFlags srcStages,
                          VkPipelineStageFlags dstStages) const noexcept;

    //  Барьер памяти. То есть разделяем поток исполнения команд плюс
    //    флашим кэши srcAccesMask и инвалидируем кэши dstAccesMask
    void memoryBarrier( VkPipelineStageFlags srcStages,
                        VkPipelineStageFlags dstStages,
                        VkAccessFlags srcAccesMask,
                        VkAccessFlags dstAccesMask) const noexcept;

    //  Image барьер. То есть барьер памяти плюс преобразование лэйаута
    //  Просто добавляет барьер в буфер команд без всякой рутины, согласований и
    //    захвата ресурсов. Без разницы, используется ли автоконтроль лэйаута
    //    или нет.
    //  ВНИМАНИЕ!!! Убедитесь, что вы не ломаете систему автоконтроля
    //    лэйаутов и обеспечиваете время жизни image до конца выполнения
    //    этого буфера
    void imageBarrier(const Image& image,
                      const ImageSlice& slice,
                      VkImageLayout srcLayout,
                      VkImageLayout dstLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags dstStages,
                      VkAccessFlags srcAccesMask,
                      VkAccessFlags dstAccesMask) const noexcept;

  private:
    void _cleanup() noexcept;

  private:
    VkCommandBuffer _handle;
    VkCommandPool _pool;
    Device& _device;
    VkCommandBufferLevel _level;

    bool _bufferInProcess;
  };

  inline VkCommandBuffer CommandBuffer::handle() const
  {
    return _handle;
  }

  inline VkCommandBufferLevel CommandBuffer::level() const
  {
    return _level;
  }

  inline bool CommandBuffer::isBufferInProcess() const noexcept
  {
    return _bufferInProcess;
  }
}