#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/Ref.h>
#include <vkr/queue/CommandBuffer.h>

namespace mt
{
  class CommandQueue;
  class Device;

  // Одновременно пул команд и буферов команд. Обертка вокруг VkCommandPool
  // И централизованное хранилище буферов команд, которые будут постоянно
  // переиспользоваться.
  // Предполагает использование отдельными сессиями. В течении сессии
  // переиспользуем или создаем(если закончились) столько буферов команд,
  // сколько потребуется. После окончания сессии ждем, когда очередь команд
  // израсходует все буферы и вызываем метод reset, который сбрасывает все
  // залоченные ресурсы всех буферов и сбрасывает VkCommandPool. Пул готов
  // к следующей сессии.
  class CommandPool
  {
  private:
    CommandPool(CommandQueue& queue);
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator = (const CommandPool&) = delete;
    virtual ~CommandPool() noexcept;

    // Взять из пула не используемый буфер или создать новый, если свободных
    // нет.
    CommandBuffer& getNextBuffer();

    // Сбросить пул комманд и все работающие с ним буферы команд
    // Убедитесь, что ни один из буферов команд не находится в исполнении
    // в очереди команд
    void reset();

  private:
    void _cleanup() noexcept;

  private:
    VkCommandPool _handle;
    Device& _device;

    using Buffers = std::vector<Ref<CommandBuffer>>;
    Buffers _buffers;
    size_t _nextBuffer;
  };
};