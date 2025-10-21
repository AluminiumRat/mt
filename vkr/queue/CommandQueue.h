#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandProducer.h>
#include <vkr/queue/QueueFamiliesInfo.h>
#include <vkr/queue/Semaphore.h>
#include <vkr/queue/SyncPoint.h>
#include <vkr/queue/TimelineSemaphore.h>

namespace mt
{
  class Device;
  class CommandBuffer;

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

    //  Добавляет инкремент встроенного таймлайн семафора в конец очереди и
    //  возвращает соответствующую пару семафор-значение
    SyncPoint createSyncPoint();

    //  Начать заполнение буфера команд.
    //  Возвращает CommandProducer, через который заполняются буферы команд.
    //  Для того, чтобы отправить команды на исполнение в очередь, необходимо
    //    вернуть продюсер в очередь через метод submitCommands
    //  Вы можете запросить несколько продюсеров и использовать их параллельно,
    //    в том числе из разных потоков. Очередность отправки команд на
    //    выполненение определяется тем, в какой очередности продюсеры будут
    //    возвращены в очередь.
    std::unique_ptr<CommandProducer> startCommands();

    //  Отправить собранный в продюсере буфер команд на исполнение и
    //    финализировать работу с продюсером.
    void submitCommands(std::unique_ptr<CommandProducer> producer);

    // Добавить место синхронизации с другой очередью. Если очереди разные, то
    // в переданную очередь будет добавлена точка синхронизации (SyncPoint), а
    // в текущую очередь (для которой вызван метод), будет добавлено ожидание
    // точки синхронизации.
    // Если обе очереди - это одна очередь, то в неё просто будет добавлен синк
    // поинт(равноситьно строгому барьеру) без ожиданий.
    void addWaitingForQueue(CommandQueue& queue,
                            VkPipelineStageFlags waitStages);

    //  Приостановить работу текущего потока пока не будут выполнены все команды
    //  из очереди
    void waitIdle() const;

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

    // Пулы команд общего назначения, из них создаются комманд продюсеры,
    // которые отдаются наружу.
    CommandPoolSet _commonPoolSet;

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