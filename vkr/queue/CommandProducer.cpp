#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandPool.h>
#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandProducer.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/VolatileDescriptorPool.h>
#include <vkr/Image.h>

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _commandBuffer(nullptr),
  _bufferInProcess(false),
  _descriptorPool(nullptr)
{
}

void CommandProducer::finalize()
{
  if(_uniformMemorySession.has_value())
  {
    _uniformMemorySession.reset();
  }

  if(_commandBuffer != nullptr && _bufferInProcess)
  {
    _bufferInProcess = false;
    if (vkEndCommandBuffer(_commandBuffer->handle()) != VK_SUCCESS)
    {
      throw std::runtime_error("CommandProducer: Failed to end command buffer.");
    }
  }
}

void CommandProducer::release(const SyncPoint& releasePoint)
{
  finalize();

  _descriptorPool = nullptr;
  _commandBuffer = nullptr;

  if(_commandPool != nullptr)
  {
    _commandPoolSet.returnPool(*_commandPool, releasePoint);
    _commandPool = nullptr;
  }
}

CommandProducer::~CommandProducer() noexcept
{
  try
  {
    if(_commandPool != nullptr)
    {
      Log::warning() << "CommandProducer::~CommandProducer(): producer is not released.";
      release(_queue.createSyncPoint());
    }
  }
  catch (std::runtime_error& exception)
  {
    Log::error() << "CommandProducer::~CommandProducer: " << exception.what();
    MT_ASSERT(false && "Unable to close CommandProducer");
  }
}

CommandBuffer& CommandProducer::_getOrCreateBuffer()
{
  if(_commandBuffer != nullptr) return *_commandBuffer;

  if(_commandPool == nullptr) _commandPool = &_commandPoolSet.getPool();
  if(_descriptorPool == nullptr)
  {
    _descriptorPool = &_commandPool->descriptorPool();
  }

  _uniformMemorySession.emplace(_commandPool->memoryPool());
  _commandBuffer = &_commandPool->getNextBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(_commandBuffer->handle(), &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("CommandProducer: Failed to begin recording command buffer.");
  }
  _bufferInProcess = true;

  return *_commandBuffer;
}

void CommandProducer::_addImageUsage( const ImageSlice& slice,
                                      VkImageLayout requiredLayout,
                                      VkPipelineStageFlags readStagesMask,
                                      VkAccessFlags readAccessMask,
                                      VkPipelineStageFlags writeStagesMask,
                                      VkAccessFlags writeAccessMask)
{
  CommandBuffer& buffer = _getOrCreateBuffer();

  // В любом случае, если используем Image, то блочим его удаление
  buffer.lockResource(slice.image());

  if(slice.image().isLayoutAutoControlEnabled())
  {
    auto insertion = _imageStates.emplace(&slice.image(), ImageLayoutState());
    bool isNewRecord = insertion.second;
    ImageLayoutState& imageState = insertion.first->second;
    if(isNewRecord)
    {
      // Image ещё не использовался в этом комманд буфере, просто заполняем
      // требования по входному лэйоуту. Преобразование будет делать очередь
      // команд при сшивании буферов команд.
      imageState.requiredIncomingLayout = requiredLayout;
      imageState.outcomingLayout = requiredLayout;
      imageState.readStagesMask = readStagesMask;
      imageState.readAccessMask = readAccessMask;
      imageState.writeStagesMask = writeStagesMask;
      imageState.writeAccessMask = writeAccessMask;
    }
    else
    {
      // Image уже использовался в этом буфере команд
      if(imageState.needToChangeLayout(slice, requiredLayout))
      {
        _addImageLayoutTransform( slice,
                                  imageState,
                                  requiredLayout,
                                  readStagesMask,
                                  readAccessMask,
                                  writeStagesMask,
                                  writeAccessMask);
      }
      else
      {
        // Не надо преобразовавать лэйауты,просто ужесточаем требования по
        // барьерам до и после буфера команд.
        imageState.writeStagesMask |= writeStagesMask;
        imageState.writeAccessMask |= writeAccessMask;
        if(!imageState.isLayoutChanged)
        {
          imageState.readStagesMask |= readStagesMask;
          imageState.readAccessMask |= readAccessMask;
        }
      }
    }
  }
}

void CommandProducer::_addImageLayoutTransform(
                                          const ImageSlice& slice,
                                          ImageLayoutState& imageState,
                                          VkImageLayout requiredLayout,
                                          VkPipelineStageFlags readStagesMask,
                                          VkAccessFlags readAccessMask,
                                          VkPipelineStageFlags writeStagesMask,
                                          VkAccessFlags writeAccessMask)
{
  CommandBuffer& buffer = _getOrCreateBuffer();

  // Для начала, если в image есть поменянный слайс - откатим его
  if(imageState.changedSlice.has_value())
  {
    buffer.imageBarrier(*imageState.changedSlice,
                        imageState.sliceLayout,
                        imageState.outcomingLayout,
                        imageState.writeStagesMask,
                        readStagesMask,
                        imageState.writeAccessMask,
                        readAccessMask);
    imageState.changedSlice.reset();
  }

  // Основное преобразование лэйаута
  buffer.imageBarrier(slice,
                      imageState.outcomingLayout,
                      requiredLayout,
                      imageState.writeStagesMask,
                      readStagesMask,
                      imageState.writeAccessMask,
                      readAccessMask);

  // Обновляем информацию по image-у
  if(slice.isSliceFull())
  {
    // Мы только что поменяли лэйаут для всего image
    imageState.outcomingLayout = requiredLayout;
  }
  else
  {
    // Мы поменяли только часть image
    imageState.changedSlice = slice;
    imageState.sliceLayout = requiredLayout;
  }

  // Мы только что прошли через барьер памяти, поэтому маски на флаш кэша
  // после буфера полностью обновляются
  imageState.writeStagesMask = writeStagesMask;
  imageState.writeAccessMask = writeAccessMask;

  imageState.isLayoutChanged = true;
}

void CommandProducer::imageBarrier(const ImageSlice& slice,
                                    VkImageLayout srcLayout,
                                    VkImageLayout dstLayout,
                                    VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  MT_ASSERT(!slice.image().isLayoutAutoControlEnabled());

  _addImageUsage( slice,
                  dstLayout,
                  dstStages,
                  dstAccesMask,
                  srcStages,
                  srcAccesMask);

  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.imageBarrier(slice,
                      srcLayout,
                      dstLayout,
                      srcStages,
                      dstStages,
                      srcAccesMask,
                      dstAccesMask);
}
