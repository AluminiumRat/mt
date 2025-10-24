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
#include <vkr/ImageSlice.h>

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _commandBuffer(nullptr),
  _descriptorPool(nullptr)
{
  try
  {
    _commandPool = &_commandPoolSet.getPool();
    _descriptorPool = &_commandPool->descriptorPool();
    _uniformMemorySession.emplace(_commandPool->memoryPool());
  }
  catch (std::exception& error)
  {
    Log::error() << "CommandProducer: " << error.what();
    MT_ASSERT(false && "Unable to create CommandProducer");
  }
}

std::optional<CommandProducer::FinalizeResult>
                                          CommandProducer::finalize() noexcept
{
  MT_ASSERT(_commandPool != nullptr);

  try
  {
    _uniformMemorySession.reset();

    if(_commandBuffer == nullptr) return std::nullopt;

    FinalizeResult result;
    result.primaryBuffer = _commandBuffer;
    _commandBuffer->endBuffer();
    _commandBuffer = nullptr;

    result.approvingBuffer = &_commandPool->getNextBuffer();
    result.imageStates = &_layoutWatcher.imageStates();

    return result;
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandProducer::finalize: " << error.what();
    MT_ASSERT(false && "Unable to finalize CommandProducer");
  }
}

void CommandProducer::release(const SyncPoint& releasePoint)
{
  finalize();

  _layoutWatcher.reset();

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
  // Нельзя создавать новый буфер после финализации
  MT_ASSERT(_uniformMemorySession.has_value());
  MT_ASSERT(_commandPool != nullptr);

  if(_commandBuffer != nullptr) return *_commandBuffer;

  _commandBuffer = &_commandPool->getNextBuffer();
  _commandBuffer->startOnetimeBuffer();

  return *_commandBuffer;
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

  CommandBuffer& buffer = _getOrCreateBuffer();

  buffer.lockResource(slice.image());

  buffer.imageBarrier(slice,
                      srcLayout,
                      dstLayout,
                      srcStages,
                      dstStages,
                      srcAccesMask,
                      dstAccesMask);
}

void CommandProducer::forceLayout(const ImageSlice& slice,
                                  VkImageLayout dstLayout,
                                  VkPipelineStageFlags readStages,
                                  VkAccessFlags readAccessMask,
                                  VkPipelineStageFlags writeStages,
                                  VkAccessFlags writeAccessMask)
{
  MT_ASSERT(slice.image().isLayoutAutoControlEnabled());

  CommandBuffer& buffer = _getOrCreateBuffer();

  buffer.lockResource(slice.image());

  _layoutWatcher.addImageUsage( slice,
                                dstLayout,
                                readStages,
                                readAccessMask,
                                writeStages,
                                writeAccessMask,
                                buffer);
}
