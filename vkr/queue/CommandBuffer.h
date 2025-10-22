#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>
#include <vkr/Ref.h>

namespace mt
{
  class Device;

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
    inline void lockResource(RefCounter& resource);

    //  Освободить все захваченные ресурсы. Необходимо вызывать, когда
    //  исполнение буфера в очереди команд уже закончено и используемые в
    //  нем ресурсы могут быть удалены.
    inline void releaseResources();

  private:
    void _cleanup() noexcept;

  private:
    VkCommandBuffer _handle;
    VkCommandPool _pool;
    Device& _device;
    VkCommandBufferLevel _level;

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

  inline void CommandBuffer::lockResource(RefCounter& resource)
  {
    _lockedResources.push_back(RefCounterReference(&resource));
  }

  inline void CommandBuffer::releaseResources()
  {
    _lockedResources.clear();
  }
}