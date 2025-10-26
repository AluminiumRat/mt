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

  _approveLayouts(*finalizeResult->approvingBuffer,
                  *finalizeResult->imageStates);

  // Собираем список буферов команд
  std::vector<VkCommandBuffer> buffersHandles;
  buffersHandles.reserve(finalizeResult->commandSequence->size());
  for(const CommandBuffer* buffer : *finalizeResult->commandSequence)
  {
    buffersHandles.push_back(buffer->handle());
  }

  //  Одновременно с сабмитом буферов продвигаем наш таймлайн семафор
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
  submitInfo.commandBufferCount = uint32_t(buffersHandles.size());
  submitInfo.pCommandBuffers = buffersHandles.data();

  if (vkQueueSubmit(handle(),
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE) != VK_SUCCESS)
  {
    MT_ASSERT(false && "CommandQueue: Failed to submit command buffer.");
  }

  producer->release(SyncPoint(*_semaphore, _lastSemaphoreValue));
}

void CommandQueue::_approveLayouts( CommandBuffer& approvingBuffer,
                                    const ImageAccessMap& imageStates) noexcept
{
  approvingBuffer.startOnetimeBuffer();
  bool barriersAdded = false;

  // Обходим все Image, которые используются в новом буфере команд, и подгоняем
  // их под требования буфера
  for(ImageAccessMap::const_iterator iState = imageStates.begin();
      iState != imageStates.end();
      iState++)
  {
    const Image* image = iState->first;
    MT_ASSERT(image->owner == nullptr || image->owner == this);
    image->owner = this;

    ImageAccess& accessInQueue = image->lastAccess;

    const ImageAccess& accessInBuffer = *iState->second.initialAccess;
    MT_ASSERT(!accessInBuffer.requiredLayouts.changedSlice.has_value());

    LayoutTranslation layoutTranslation = getLayoutTranslation(
                                  accessInQueue.requiredLayouts,
                                  ImageSlice(*image),
                                  accessInBuffer.requiredLayouts.primaryLayout);
    if(layoutTranslation.translationType.needToDoAnything() ||
        accessInQueue.memoryAccess.needBarrier(accessInBuffer.memoryAccess))
    {
      // Надо добавить барьер, воспользуемся готовым функционалом
      ApprovingPoint approvingPoint{
                        .approvingBuffer = &approvingBuffer,
                        .previousAccess = accessInQueue,
                        .layoutTranslation = layoutTranslation.translationType};
      approvingPoint.makeApprove(*image, accessInBuffer);

      // После барьера текущий доступ к Image полностью обновляется
      accessInQueue = accessInBuffer;

      barriersAdded = true;
    }
    else
    {
      // Барьер не нужен, дополняем текущий доступ доступом из буфера
      accessInQueue.memoryAccess.merge(accessInBuffer.memoryAccess);
    }
  }

  approvingBuffer.endBuffer();

  // Если был записан хотябы один барьер, то сабмитим буфер команд
  if(approvingBuffer.isBufferInProcess())
  {
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

