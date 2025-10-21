#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>

#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>

using namespace mt;

CommandQueue::CommandQueue( Device& device,
                            uint32_t familyIndex,
                            uint32_t queueIndex,
                            const QueueFamily& family,
                            std::recursive_mutex& commonMutex) :
  _handle(VK_NULL_HANDLE),
  _device(device),
  _family(family),
  _semaphore(new TimelineSemaphore(0, device)),
  _lastSemaphoreValue(0),
  _commonPoolSet(*this),
  _commonMutex(commonMutex)
{
  try
  {
    vkGetDeviceQueue( device.handle(),
                      familyIndex,
                      queueIndex,
                      &_handle);
    MT_ASSERT((_handle != VK_NULL_HANDLE) && "CommandQueue::CommandQueue: handle is null")
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

CommandQueue::~CommandQueue() noexcept
{
  _cleanup();
}

void CommandQueue::_cleanup() noexcept
{
  vkQueueWaitIdle(_handle);
}

void CommandQueue::addSignalSemaphore(Semaphore& semaphore)
{
  std::lock_guard lock(_commonMutex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 0;
  submitInfo.pCommandBuffers = nullptr;
  submitInfo.signalSemaphoreCount = 1;
  VkSemaphore signalSemaphores[] = { semaphore.handle() };
  submitInfo.pSignalSemaphores = signalSemaphores;
  if (vkQueueSubmit(_handle,
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit semaphore signal");
  }
}

void CommandQueue::addWaitForSemaphore(
  Semaphore& semaphore,
  VkPipelineStageFlags waitStages)
{
  std::lock_guard lock(_commonMutex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  VkSemaphore waitSemaphores[] = { semaphore.handle() };
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = &waitStages;
  submitInfo.commandBufferCount = 0;
  submitInfo.pCommandBuffers = nullptr;
  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit semaphore wait");
  }
}

SyncPoint CommandQueue::createSyncPoint()
{
  std::lock_guard lock(_commonMutex);

  _lastSemaphoreValue++;

  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &_lastSemaphoreValue;

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = &timelineInfo;
  submitInfo.signalSemaphoreCount  = 1;
  VkSemaphore semaphore = _semaphore->handle();
  submitInfo.pSignalSemaphores = &semaphore;

  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit timeline semaphore increment.");
  }

  return SyncPoint(*_semaphore, _lastSemaphoreValue);
}

std::unique_ptr<CommandProducer> CommandQueue::startCommands()
{
  std::lock_guard lock(_commonMutex);
  return std::make_unique<CommandProducer>(_commonPoolSet);
}

void CommandQueue::submitCommands(std::unique_ptr<CommandProducer> producer)
{
  MT_ASSERT(producer != nullptr);
  MT_ASSERT(&producer->queue() == this);

  // Проверка на случай, если продюсер был запрошен, но команды в него не
  // уходили
  CommandBuffer* commandBuffer = producer->commandBuffer();
  if (commandBuffer == nullptr) return;

  // Перед тем как отправлять команды на исполнение, необходимо гарантировать,
  // что юниформ буферы, которые они используют, доступны на ГПУ
  producer->flushUniformData();

  std::lock_guard lock(_commonMutex);

  //  Одновременно с сабмитом буфера продвигаем наш таймлайн семафор
  //  для того чтобы получить синк поинт, на котором можно чистить пулы
  _lastSemaphoreValue++;
  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &_lastSemaphoreValue;

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = &timelineInfo;
  submitInfo.signalSemaphoreCount  = 1;
  VkSemaphore semaphore = _semaphore->handle();
  submitInfo.pSignalSemaphores = &semaphore;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer bufferHandle = commandBuffer->handle();
  submitInfo.pCommandBuffers = &bufferHandle;

  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit timeline semaphore increment.");
  }

  producer->release(SyncPoint(*_semaphore, _lastSemaphoreValue));
}

void CommandQueue::addWaitingForQueue(
  CommandQueue& queue,
  VkPipelineStageFlags waitStages)
{
  std::lock_guard lock(_commonMutex);

  if(&queue == this)
  {
    createSyncPoint();
  }
  else
  {
    SyncPoint syncPoint = queue.createSyncPoint();
    _addWaitingForSyncPoint(syncPoint, waitStages);
  }
}

void CommandQueue::_addWaitingForSyncPoint(
  const SyncPoint& syncPoint,
  VkPipelineStageFlags waitStages)
{
  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.pNext = NULL;
  timelineInfo.waitSemaphoreValueCount = 1;
  uint64_t semaphoreValue = syncPoint.waitingValue();
  timelineInfo.pWaitSemaphoreValues = &semaphoreValue;

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = &timelineInfo;
  submitInfo.waitSemaphoreCount = 1;
  VkSemaphore semaphore = syncPoint.semaphore().handle();
  submitInfo.pWaitSemaphores = &semaphore;
  submitInfo.pWaitDstStageMask = &waitStages;

  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit waiting for the timeline semaphore.");
  }
}

void CommandQueue::waitIdle() const
{
  std::lock_guard lock(_commonMutex);

  if(vkQueueWaitIdle(_handle) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to wait device queue.");
  }
}

