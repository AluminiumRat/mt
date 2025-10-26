#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>

#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/Image.h>

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

  std::lock_guard lock(_commonMutex);

  std::optional<CommandProducer::FinalizeResult> finalizeResult =
                                                          producer->finalize();
  if(!finalizeResult.has_value()) return;

  //_approveLayouts(*finalizeResult->approvingBuffer,
  //                *finalizeResult->imageStates);

  //  Одновременно с сабмитом буфера продвигаем наш таймлайн семафор
  //  для того чтобы получить синк поинт, на котором можно чистить пулы
  /*_lastSemaphoreValue++;
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
  VkCommandBuffer bufferHandle = finalizeResult->primaryBuffer->handle();
  submitInfo.pCommandBuffers = &bufferHandle;

  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    MT_ASSERT(false && "CommandQueue: Failed to submit command buffer.");
  }

  producer->release(SyncPoint(*_semaphore, _lastSemaphoreValue));*/
}

void CommandQueue::_approveLayouts(
                                CommandBuffer& approvingBuffer,
                                const ImageLayoutStateSet& imageStates) noexcept
{
  // Барьеры будем выдавать пачками максимум по 128 штук
  VkImageMemoryBarrier barriers[128];
  uint32_t barrierCount = 0;
  VkPipelineStageFlags srcStages = 0;
  VkPipelineStageFlags dstStages = 0;

  auto flushBarriers = [&]()
    {
      if (barrierCount != 0)
      {
        if(!approvingBuffer.isBufferInProcess())
        {
          approvingBuffer.startOnetimeBuffer();
        }

        vkCmdPipelineBarrier( approvingBuffer.handle(),
                              srcStages,
                              dstStages,
                              0,
                              0,
                              nullptr,
                              0,
                              nullptr,
                              barrierCount,
                              barriers);
        barrierCount = 0;
        srcStages = 0;
        dstStages = 0;
      }
    };

  // Обходим все Image, которые используются в новом буфере команд, и подгоняем
  // их под требования буфера
  for(ImageLayoutStateSet::const_iterator iState = imageStates.begin();
      iState != imageStates.end();
      iState++)
  {
    const Image* image = iState->first;
    MT_ASSERT(image->owner == nullptr || image->owner == this);
    image->owner = this;

    const ImageLayoutStateInBuffer& stateInBuffer = iState->second;
    ImageLayoutStateInQueue& stateInQueue = image->layoutState;

    bool barrierIsAdded = false;

    // Если в image-е есть посторонний слайс, то откатываем его к общему layout-у
    if(stateInQueue.changedSlice.has_value())
    {
      barriers[barrierCount].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barriers[barrierCount].oldLayout = stateInQueue.sliceLayout;
      barriers[barrierCount].newLayout = stateInQueue.layout;
      barriers[barrierCount].image = image->handle();
      barriers[barrierCount].subresourceRange =
                                        stateInQueue.changedSlice->makeRange();
      barriers[barrierCount].srcAccessMask = stateInQueue.writeAccessMask;
      barriers[barrierCount].dstAccessMask = stateInBuffer.readAccessMask;

      srcStages |= stateInQueue.writeStagesMask;
      dstStages |= stateInBuffer.readStagesMask;

      barrierCount++;
      barrierIsAdded = true;

      if(barrierCount == 128) flushBarriers();
    }

    // Если у Image неподходящий лэйаут, то поменяем его
    if(stateInQueue.layout != stateInBuffer.requiredIncomingLayout)
    {
      barriers[barrierCount].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barriers[barrierCount].oldLayout = stateInQueue.layout;
      barriers[barrierCount].newLayout = stateInBuffer.requiredIncomingLayout;
      barriers[barrierCount].image = image->handle();
      barriers[barrierCount].subresourceRange = ImageSlice(*image).makeRange();
      barriers[barrierCount].srcAccessMask = stateInQueue.writeAccessMask;
      barriers[barrierCount].dstAccessMask = stateInBuffer.readAccessMask;

      srcStages |= stateInQueue.writeStagesMask;
      dstStages |= stateInBuffer.readStagesMask;

      barrierCount++;
      barrierIsAdded = true;

      if (barrierCount == 128) flushBarriers();
    }

    // Сохраним информацию об изменениях, произошедших в буфере
    stateInQueue.layout = stateInBuffer.outcomingLayout;
    if(barrierIsAdded)
    {
      stateInQueue.writeStagesMask = stateInBuffer.writeStagesMask;
      stateInQueue.writeAccessMask = stateInBuffer.writeAccessMask;
    }
    else
    {
      stateInQueue.writeStagesMask |= stateInBuffer.writeStagesMask;
      stateInQueue.writeAccessMask |= stateInBuffer.writeAccessMask;
    }
    stateInQueue.changedSlice = stateInBuffer.changedSlice;
    stateInQueue.sliceLayout = stateInBuffer.sliceLayout;
  }

  flushBarriers();

  // Если к этому месту буфер стартован в работу, значит его надо засабмитить
  if(approvingBuffer.isBufferInProcess())
  {
    approvingBuffer.endBuffer();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer bufferHandle = approvingBuffer.handle();
    submitInfo.pCommandBuffers = &bufferHandle;

    if (vkQueueSubmit(handle(),
                      1,
                      &submitInfo,
                      VK_NULL_HANDLE) != VK_SUCCESS)
    {
      MT_ASSERT(false && "CommandQueue: Failed to submit approving command buffer.");
    }
  }
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

