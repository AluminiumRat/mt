#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>

#include <vkr/queue/CommandQueue.h>
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

  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.signalSemaphoreValueCount = 1;
  uint64_t nextSemaphoreValue = _lastSemaphoreValue + 1;
  timelineInfo.pSignalSemaphoreValues = &nextSemaphoreValue;

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

  _lastSemaphoreValue = nextSemaphoreValue;

  return SyncPoint{ .semaphore = _semaphore,
                    .semaphoreValue = nextSemaphoreValue};
}

void CommandQueue::addWaitingForSyncPoint(const SyncPoint& syncPoint)
{
  MT_ASSERT (syncPoint.semaphore != nullptr);

  // Это синкпоинт этой же очереди, не надо ничего ждать
  if (syncPoint.semaphore.get() == _semaphore) return;

  std::lock_guard lock(_commonMutex);

  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.pNext = NULL;
  timelineInfo.waitSemaphoreValueCount = 1;
  timelineInfo.pWaitSemaphoreValues = &syncPoint.semaphoreValue;

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = &timelineInfo;
  submitInfo.waitSemaphoreCount = 1;
  VkSemaphore semaphore = syncPoint.semaphore->handle();
  submitInfo.pWaitSemaphores = &semaphore;

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

/*std::unique_ptr<CommandProducer> CommandQueue::startCommands()
{
  size_t attemptCount = 0;
  while(!_pools[_nextPoolIndex]._available)
  {
    attemptCount++;
    if(attemptCount == _pools.size()) Abort("CommandQueue::startCommands: unable to find free pool.");
    _nextPoolIndex = (_nextPoolIndex + 1) % _pools.size();
  };
  
  Fence& waitFence = *_pools[_nextPoolIndex].fence;
  waitFence.wait();

  _pools[_nextPoolIndex]._available = false;
  
  CommandPool& pool = *_pools[_nextPoolIndex].pool;
  std::unique_ptr<CommandProducer> producer(new Producer( pool,
                                                          *this,
                                                          _nextPoolIndex));
  _nextPoolIndex = (_nextPoolIndex + 1) % _pools.size();
  return std::move(producer);
}*/

/*Fence& CommandQueue::submit(std::unique_ptr<CommandProducer> commandProducer,
                            Semaphore* startSemaphore,
                            VkPipelineStageFlags dstStageMask,
                            Semaphore* endSignalSemaphore)
{
  Producer& producer = static_cast<Producer&>(*commandProducer);
  if(&producer._queue != this) Abort("CommandQueue::submit: command producer was created in a different queue.");

  Fence& waitFence = *_pools[producer._poolIndex].fence;

  if(commandProducer->bufferHandle() == VK_NULL_HANDLE)
  {
    commandProducer->startNewBuffer();
  }
  commandProducer->submitCurrentBuffer( startSemaphore,
                                        dstStageMask,
                                        endSignalSemaphore,
                                        &waitFence);

  return waitFence;
}*/

/*void CommandQueue::submit(CommandBuffer& buffer,
                          Semaphore* startSemaphore,
                          VkPipelineStageFlags dstStageMask,
                          Semaphore* endSignalSemaphore,
                          Fence* endSignalFence)
{
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  submitInfo.waitSemaphoreCount = 0;
  VkSemaphore waitSemaphores[] = {VK_NULL_HANDLE};
  if(startSemaphore != nullptr)
  {
    submitInfo.waitSemaphoreCount = 1;
    waitSemaphores[0] = { startSemaphore->handle()};
    buffer.lockResource(*startSemaphore);
  }
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = &dstStageMask;

  submitInfo.commandBufferCount = 1;
  VkCommandBuffer bufferHandle = buffer.handle();
  submitInfo.pCommandBuffers = &bufferHandle;
  VkSemaphore signalSemaphores[] = {VK_NULL_HANDLE};
  submitInfo.signalSemaphoreCount = 0;
  if(endSignalSemaphore != nullptr)
  {
    submitInfo.signalSemaphoreCount = 1;
    signalSemaphores[0] = endSignalSemaphore->handle();
    buffer.lockResource(*endSignalSemaphore);
  }
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkFence fenceHandle = VK_NULL_HANDLE;
  if(endSignalFence != nullptr)
  {
    fenceHandle = endSignalFence->handle();
    endSignalFence->reset();
  }
  
  if (vkQueueSubmit(_handle,
                    1,
                    &submitInfo,
                    fenceHandle) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to submit command buffer.");
  }
}*/
