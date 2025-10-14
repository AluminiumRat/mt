#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

//#include <mtt/render/CommandQueue/CommandPool.h>
//#include <mtt/render/CommandQueue/CommandProducer.h>
//#include <mtt/render/CommandQueue/Fence.h>
#include <vkr/queue/QueueFamiliesInfo.h>
#include "vkr/queue/Semaphore.h"
//#include <mtt/render/Ref.h>

namespace mt
{
  class Device;
  //class Semaphore;

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
    void addWaitSemaphore(Semaphore& semaphore,
                          VkPipelineStageFlags waitStages);

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

  private:
    VkQueue _handle;
    Device& _device;

    QueueFamily _family;

    //size_t _nextPoolIndex;

    //using Resources = std::vector<LockableReference>;
    //Resources _attachedResources;
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