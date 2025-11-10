#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>

#include <vkr/image/Image.h>
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
  commonPoolSet(*this),
  commonMutex(commonMutex),
  _handle(VK_NULL_HANDLE),
  _device(device),
  _family(family),
  _semaphore(new TimelineSemaphore(0, device)),
  _lastSemaphoreValue(0)
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
  std::lock_guard lock(commonMutex);

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
  std::lock_guard lock(commonMutex);

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
  std::lock_guard lock(commonMutex);

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
  std::lock_guard lock(commonMutex);
  return std::make_unique<CommandProducer>(commonPoolSet);
}

void CommandQueue::submitCommands(std::unique_ptr<CommandProducer> producer)
{
  MT_ASSERT(producer != nullptr);
  MT_ASSERT(&producer->queue() == this);

  std::lock_guard lock(commonMutex);

  std::optional<CommandProducer::FinalizeResult> finalizeResult =
                                                          producer->finalize();
  if(!finalizeResult.has_value()) return;

  _matchLayouts(*finalizeResult->matchingBuffer,
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

void CommandQueue::_matchLayouts( CommandBuffer& matchingBuffer,
                                  const ImageAccessMap& imageStates) noexcept
{
  if(imageStates.empty()) return;

  matchingBuffer.startOnetimeBuffer();
  bool barriersAdded = false;

  //  Обходим все Image, которые используются в новой последовательности команд,
  //  и подгоняем их под требования буфера
  for(ImageAccessMap::const_iterator iState = imageStates.begin();
      iState != imageStates.end();
      iState++)
  {
    const Image* image = iState->first;
    MT_ASSERT(image->owner == nullptr || image->owner == this);
    image->owner = this;

    // Если Image ещё нигде не использовался - инициализируем лэйаут.
    if(!image->lastAccess.has_value())
    {
      matchingBuffer.imageBarrier(*image,
                                  ImageSlice(*image),
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_GENERAL,
                                  0,0,0,0);
      image->lastAccess = ImageAccess();
    }

    ImageAccess& accessInQueue = *image->lastAccess;
    const ImageAccess& accessInBuffer = *iState->second.initialAccess;
    ImageAccess::TransformHint transformHint =
                              accessInQueue.hasSameLayouts(accessInBuffer) ?
                                                    ImageAccess::REUSE_SLICES :
                                                    ImageAccess::RESET_SLICES;
    MatchingPoint accessMatcher{.matchingBuffer = &matchingBuffer,
                                .previousAccess = accessInQueue,
                                .transformHint = transformHint};
    accessMatcher.makeMatch(*image, accessInBuffer);
    accessInQueue = iState->second.currentAccess;
    barriersAdded = true;
  }

  matchingBuffer.endBuffer();

  // Если был записан хотябы один барьер, то сабмитим буфер команд
  if(barriersAdded)
  {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer bufferHandle = matchingBuffer.handle();
    submitInfo.pCommandBuffers = &bufferHandle;

    if (vkQueueSubmit(handle(),
                      1,
                      &submitInfo,
                      VK_NULL_HANDLE) != VK_SUCCESS)
    {
      MT_ASSERT(false && "CommandQueue: Failed to submit matching command buffer.");
    }
  }
}

void CommandQueue::addWaitingForQueue(
  CommandQueue& queue,
  VkPipelineStageFlags waitStages)
{
  std::lock_guard lock(commonMutex);

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
  std::lock_guard lock(commonMutex);

  if(vkQueueWaitIdle(_handle) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandQueue: Failed to wait device queue.");
  }
}

void CommandQueue::_makeFullMemoryBarrier()
{
  std::unique_ptr<CommandProducer> producer = startCommands();
  producer->memoryBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_ACCESS_MEMORY_WRITE_BIT,
                          VK_ACCESS_MEMORY_READ_BIT);
  submitCommands(std::move(producer));
}

SyncPoint CommandQueue::ownershipTransfer(CommandQueue& oldQueue,
                                          CommandQueue& newQueue,
                                          const Image& image)
{
  MT_ASSERT(image.owner == nullptr || image.owner == &oldQueue);

  std::lock_guard lock(oldQueue.commonMutex);

  if(oldQueue.family().index() == newQueue.family().index())
  {
    // Трансфер не нужен, просто ставим полный барьер памяти
    oldQueue._makeFullMemoryBarrier();
    SyncPoint syncPoint = oldQueue.createSyncPoint();
    image.owner = &newQueue;
    return syncPoint;
  }

  // Первая половина трансфера - освобождаем владение старой очередью
  std::unique_ptr<CommandProducer> releaseProducer = oldQueue.startCommands();
  releaseProducer->halfOwnershipTransfer( image,
                                          oldQueue.family().index(),
                                          newQueue.family().index());
  oldQueue.submitCommands(std::move(releaseProducer));
  image.owner = nullptr;

  // У нас уже есть половина трансфера, который мы не можем откатить. Любые
  // ошибки во второй половине трансфера приводят к пату
  try
  {
    // Вторая половина трансфера - захватываем владение новой очередью
    newQueue.addWaitingForQueue(oldQueue, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    std::unique_ptr<CommandProducer> acquireProducer = newQueue.startCommands();
    acquireProducer->halfOwnershipTransfer( image,
                                            oldQueue.family().index(),
                                            newQueue.family().index());
    newQueue.submitCommands(std::move(acquireProducer));
    image.owner = &newQueue;
    return newQueue.createSyncPoint();
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandQueue::ownershipTransfer: " << error.what();
    Abort("CommandQueue::ownershipTransfer: unable to acquire image ownership");
  }
}

SyncPoint CommandQueue::ownershipTransfer(CommandQueue& oldQueue,
                                          CommandQueue& newQueue,
                                          const DataBuffer& buffer)
{
  std::lock_guard lock(oldQueue.commonMutex);

  if(oldQueue.family().index() == newQueue.family().index())
  {
    // Трансфер не нужен, просто ставим полный барьер памяти
    oldQueue._makeFullMemoryBarrier();
    SyncPoint syncPoint = oldQueue.createSyncPoint();
    return syncPoint;
  }

  // Первая половина трансфера - освобождаем владение старой очередью
  std::unique_ptr<CommandProducer> releaseProducer = oldQueue.startCommands();
  releaseProducer->halfOwnershipTransfer( buffer,
                                          oldQueue.family().index(),
                                          newQueue.family().index());
  oldQueue.submitCommands(std::move(releaseProducer));

  // У нас уже есть половина трансфера, который мы не можем откатить. Любые
  // ошибки во второй половине трансфера приводят к пату
  try
  {
    // Вторая половина трансфера - захватываем владение новой очередью
    newQueue.addWaitingForQueue(oldQueue, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    std::unique_ptr<CommandProducer> acquireProducer = newQueue.startCommands();
    acquireProducer->halfOwnershipTransfer( buffer,
                                            oldQueue.family().index(),
                                            newQueue.family().index());
    newQueue.submitCommands(std::move(acquireProducer));
    return newQueue.createSyncPoint();
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandQueue::ownershipTransfer: " << error.what();
    Abort("CommandQueue::ownershipTransfer: unable to acquire buffer ownership");
  }
}