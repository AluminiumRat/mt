#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>
#include <util/Ref.h>

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

    //  Захватить владение ресурсом. Это продляет жизнь ресурса и позволяет
    //  предотвратить его удаление, пока буфер находится на исполнении в
    //  очереди команд
    inline void lockResource(const RefCounter& resource);

    //  Освободить все захваченные ресурсы. Необходимо вызывать, когда
    //  исполнение буфера в очереди команд уже закончено и используемые в
    //  нем ресурсы могут быть удалены.
    inline void releaseResources();

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

    std::vector<RefCounterReference> _lockedResources;
  };

  inline VkCommandBuffer CommandBuffer::handle() const
  {
    return _handle;
  }

  inline VkCommandBufferLevel CommandBuffer::level() const
  {
    return _level;
  }

  inline void CommandBuffer::lockResource(const RefCounter& resource)
  {
    _lockedResources.push_back(RefCounterReference(&resource));
  }

  inline void CommandBuffer::releaseResources()
  {
    _lockedResources.clear();
  }

  inline bool CommandBuffer::isBufferInProcess() const noexcept
  {
    return _bufferInProcess;
  }
}