#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/queue/QueueFamiliesInfo.h>
#include <vkr/queue/Semaphore.h>
#include <vkr/queue/SyncPoint.h>
#include <vkr/queue/TimelineSemaphore.h>

namespace mt
{
  class Device;

  // Класс отвечает за отправку команд на GPU. По факту - толстая обертка вокруг
  // VkQueue
  class CommandQueue
  {
  private:
    // Логический девайс создает очереди в конструкторе. Это единственный
    // способ создания очередей.
    friend class Device;
    CommandQueue( Device& device,
                  uint32_t familyIndex,
                  uint32_t queueIndex,
                  const QueueFamily& family,
                  std::recursive_mutex& commonMutex);
  public:
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator = (const CommandQueue&) = delete;
    ~CommandQueue() noexcept;

    inline VkQueue handle() const;
    inline Device& device() const;
    inline const QueueFamily& family() const;

    // Добавить команду, переводящую семафор в состояние "signaled"
    void addSignalSemaphore(Semaphore& semaphore);
    // ВНИМАНИЕ!!! Очередь не захватывает владение семафором! Вызовите
    // waitIdle перед удалением семафора.
    void addWaitForSemaphore( Semaphore& semaphore,
                              VkPipelineStageFlags waitStages);

    SyncPoint createSyncPoint();

    /*std::unique_ptr<CommandProducer> startCommands();
    /// You should use producer that was created from this queue
    /// Returns fence to wait end of command execution.
    Fence& submit(std::unique_ptr<CommandProducer> commandProducer,
                  Semaphore* startSemaphore,
                  VkPipelineStageFlags dstStageMask,
                  Semaphore* endSignalSemaphore);

    void submit(CommandBuffer& buffer,
                Semaphore* startSemaphore,
                VkPipelineStageFlags dstStageMask,
                Semaphore* endSignalSemaphore,
                Fence* endSignalFence);*/

    // Добавить место синхронизации с другой очередью. Если очереди разные, то
    // в переданную очередь будет добавлена точка синхронизации (SyncPoint), а
    // в текущую очередь (для которой вызван метод), будет добавлено ожидание
    // точки синхронизации.
    // Если обе очереди - это одна очередь, то в неё просто будет добавлен синк
    // поинт(равноситьно строгому барьеру) без ожиданий.
    void addWaitingForQueue(CommandQueue& queue,
                            VkPipelineStageFlags waitStages);

    void waitIdle() const;

  private:
    /*class Producer : public CommandProducer
    {
    public:
      Producer( CommandPool& pool,
                CommandQueue& queue,
                size_t poolIndex);
      Producer(const Producer&) = delete;
      Producer& operator = (const Producer&) = delete;
      virtual ~Producer();

      CommandQueue& _queue;
      size_t _poolIndex;
    };*/

  private:
    void _cleanup() noexcept;
    // ВНИМАНИЕ!!! Этот метод не захватывает владение семафором, он работает
    // только с семафорами очередей и предполагает, что семафоры будут удалены
    // только вместе с очередями и девайсом.
    void _addWaitingForSyncPoint( const SyncPoint& syncPoint,
                                  VkPipelineStageFlags waitStages);

  private:
    VkQueue _handle;
    Device& _device;

    QueueFamily _family;

    // Основной семафор очереди команд, существующий на протяжении всей
    // жизни очереди. Основное средство синхронизации очередей между собой.
    Ref<TimelineSemaphore> _semaphore;
    uint64_t _lastSemaphoreValue;

    // Мьютекс, общий для всех очередей одного логического устройства.
    // Служит для синхронизации межпотока между очередями.
    std::recursive_mutex& _commonMutex;
  };

  inline VkQueue CommandQueue::handle() const
  {
    return _handle;
  }

  inline Device& CommandQueue::device() const
  {
    return _device;
  }

  inline const QueueFamily& CommandQueue::family() const
  {
    return _family;
  }
}