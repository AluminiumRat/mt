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
  _currentPrimaryBuffer(nullptr),
  _currentMatchingBuffer(nullptr),
  _descriptorPool(nullptr),
  _isFinalized(false)
{
  try
  {
    _commandPool = &_commandPoolSet.getPool();
    _descriptorPool = &_commandPool->descriptorPool();
    _uniformMemorySession.emplace(_commandPool->memoryPool());
    _commandSequence.reserve(1024);
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
  MT_ASSERT(!_isFinalized);

  try
  {
    _isFinalized = true;

    _uniformMemorySession.reset();

    if(_commandSequence.empty()) return std::nullopt;

    for(CommandBuffer* buffer : _commandSequence)
    {
      buffer->endBuffer();
    }

    if(_currentMatchingBuffer == nullptr)
    {
      _currentMatchingBuffer = &_commandPool->getNextBuffer();
    }

    FinalizeResult result;
    result.commandSequence = &_commandSequence;
    result.matchingBuffer = _currentMatchingBuffer;
    result.imageStates = &_accessWatcher.finalize();

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
  if(!_isFinalized) finalize();

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
  MT_ASSERT(!_isFinalized);

  if(_currentPrimaryBuffer != nullptr) return *_currentPrimaryBuffer;

  _currentPrimaryBuffer = &_commandPool->getNextBuffer();
  _currentPrimaryBuffer->startOnetimeBuffer();
  _commandSequence.push_back(_currentPrimaryBuffer);

  return *_currentPrimaryBuffer;
}

void CommandProducer::_addImageUsage( const Image& image,
                                      const SliceAccess& sliceAccess)
{
  if(!image.isLayoutAutoControlEnabled())
  {
    return;
  }

  if(_currentMatchingBuffer == nullptr)
  {
    _currentMatchingBuffer = &_commandPool->getNextBuffer();
  }

  if(_accessWatcher.addImageAccess( image,
                                    sliceAccess,
                                    *_currentMatchingBuffer) ==
                                          ImageAccessWatcher::NEED_TO_MATCHING)
  {
    // Обнаружили место, где будет барьер
    CommandBuffer* matchingBuffer = _currentMatchingBuffer;
    _currentMatchingBuffer = nullptr;
    _currentPrimaryBuffer = nullptr;
    matchingBuffer->startOnetimeBuffer();
    _commandSequence.push_back(matchingBuffer);
  }
}

void CommandProducer::imageBarrier( const Image& image,
                                    const ImageSlice& slice,
                                    VkImageLayout srcLayout,
                                    VkImageLayout dstLayout,
                                    VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  MT_ASSERT(!image.isLayoutAutoControlEnabled());

  CommandBuffer& buffer = _getOrCreateBuffer();

  buffer.lockResource(image);

  buffer.imageBarrier(image,
                      slice,
                      srcLayout,
                      dstLayout,
                      srcStages,
                      dstStages,
                      srcAccesMask,
                      dstAccesMask);
}

void CommandProducer::forceLayout(const Image& image,
                                  const ImageSlice& slice,
                                  VkImageLayout dstLayout,
                                  VkPipelineStageFlags readStages,
                                  VkAccessFlags readAccessMask,
                                  VkPipelineStageFlags writeStages,
                                  VkAccessFlags writeAccessMask)
{
  MT_ASSERT(image.isLayoutAutoControlEnabled());

  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.lockResource(image);

  _addImageUsage( image,
                  SliceAccess{.slice = slice,
                              .requiredLayout = dstLayout,
                              .memoryAccess = MemoryAccess{
                                        .readStagesMask = readStages,
                                        .readAccessMask = readAccessMask,
                                        .writeStagesMask = writeStages,
                                        .writeAccessMask = writeAccessMask}});
}
