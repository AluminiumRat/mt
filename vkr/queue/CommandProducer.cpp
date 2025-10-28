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
  _cachedMatchingBuffer(nullptr),
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

    FinalizeResult result;
    result.commandSequence = &_commandSequence;
    result.matchingBuffer = _cachedMatchingBuffer != nullptr ?
                                                _cachedMatchingBuffer :
                                                &_commandPool->getNextBuffer();
    result.imageStates = &_accessWatcher.finalize();

    //  EndBuffer стоит после _accessWatcher.finalize, так как finalize
    //  ещё может писать в буферы команд(согласование предпоследнего и
    //  последнего доступов)
    for(CommandBuffer* buffer : _commandSequence)
    {
      buffer->endBuffer();
    }

    return result;
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandProducer::finalize: " << error.what();
    Abort("Unable to finalize CommandProducer");
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
                                      const ImageAccess& access)
{
  if(image.isLayoutAutoControlEnabled())
  {
    if(_cachedMatchingBuffer == nullptr)
    {
      _cachedMatchingBuffer = &_commandPool->getNextBuffer();
    }

    if(_accessWatcher.addImageAccess( image,
                                      access,
                                      *_cachedMatchingBuffer) ==
                                          ImageAccessWatcher::NEED_TO_MATCHING)
    {
      // Обнаружили место, где будет барьер
      CommandBuffer* matchingBuffer = _cachedMatchingBuffer;
      _cachedMatchingBuffer = nullptr;
      _currentPrimaryBuffer = nullptr;
      matchingBuffer->startOnetimeBuffer();
      _commandSequence.push_back(matchingBuffer);
    }
  }

  // Захватываем владение, чтобы Image не удалился во время выполнения буфера
  // Захватываем уже новым буфером команд после барьера
  CommandBuffer& buffer = _getOrCreateBuffer();
  buffer.lockResource(image);
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

  ImageAccess imageAccess;
  imageAccess.slices[0] = slice;
  imageAccess.layouts[0] = dstLayout;
  imageAccess.memoryAccess[0] = MemoryAccess{
                                  .readStagesMask = dstStages,
                                  .readAccessMask = dstAccesMask,
                                  .writeStagesMask = dstStages,
                                  .writeAccessMask = dstAccesMask};

  _addImageUsage(image, imageAccess);

  CommandBuffer& buffer = _getOrCreateBuffer();
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

  ImageAccess imageAccess;
  imageAccess.slices[0] = slice;
  imageAccess.layouts[0] = dstLayout;
  imageAccess.memoryAccess[0] = MemoryAccess{
                                  .readStagesMask = readStages,
                                  .readAccessMask = readAccessMask,
                                  .writeStagesMask = writeStages,
                                  .writeAccessMask = writeAccessMask};
  imageAccess.slicesCount = 1;

  _addImageUsage( image, imageAccess);
}
