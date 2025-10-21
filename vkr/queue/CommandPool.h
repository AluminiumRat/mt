#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/Ref.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/UniformMemoryPool.h>
#include <vkr/queue/VolatileDescriptorPool.h>

namespace mt
{
  class CommandQueue;
  class Device;

  //  Пул всего необходимого для того чтобы полноценно построить буфер команд.
  //    Здесь и обертка вокруг VkCommandPool, и пул объектов CommandBuffer, и
  //    пул юниформ буферов и пул дескриптеров.
  //  Предполагает использование отдельными сессиями. В течении сессии
  //    переиспользуем или создаем(если закончились) столько ресурсов,
  //    сколько потребуется. После окончания сессии ждем, когда очередь команд
  //    пройдет все буферы и вызываем метод reset, который сбрасывает все
  //    залоченные ресурсы всех буферов и сбрасывает VkCommandPool. Пул готов
  //    к следующей сессии.
  class CommandPool
  {
  public:
    CommandPool(CommandQueue& queue);
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator = (const CommandPool&) = delete;
    virtual ~CommandPool() noexcept;

    inline UniformMemoryPool& memoryPool() noexcept;
    inline const UniformMemoryPool& memoryPool() const noexcept;

    inline VolatileDescriptorPool& descriptorPool() noexcept;
    inline const VolatileDescriptorPool& descriptorPool() const noexcept;

    // Взять из пула не используемый буфер или создать новый, если свободных
    // нет.
    CommandBuffer& getNextBuffer();

    //  Сбросить пул комманд и все работающие с ним буферы команд и
    //    волатильные дескриптер-сеты
    //  ВНИМАНИЕ!!! Убедитесь, что ни один из буферов команд не находится
    //    в очереди команд
    void reset();

  private:
    void _cleanup() noexcept;

  private:
    VkCommandPool _handle;
    Device& _device;

    UniformMemoryPool _memoryPool;
    VolatileDescriptorPool _descriptorPool;

    using Buffers = std::vector<Ref<CommandBuffer>>;
    Buffers _buffers;
    size_t _nextBuffer;
  };

  inline UniformMemoryPool& CommandPool::memoryPool() noexcept
  {
    return _memoryPool;
  }

  inline const UniformMemoryPool& CommandPool::memoryPool() const noexcept
  {
    return _memoryPool;
  }

  inline VolatileDescriptorPool& CommandPool::descriptorPool() noexcept
  {
    return _descriptorPool;
  }

  inline const VolatileDescriptorPool&
                                  CommandPool::descriptorPool() const noexcept
  {
    return _descriptorPool;
  }
};