#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImageSlice.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandProducer.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/VolatileDescriptorPool.h>

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _currentPrimaryBuffer(nullptr),
  _preparationBuffer(nullptr),
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

void CommandProducer::finalizeCommands() noexcept
{
}

std::optional<CommandProducer::FinalizeResult>
                                          CommandProducer::finalize() noexcept
{
  MT_ASSERT(!_isFinalized);

  finalizeCommands();

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

void CommandProducer::halfOwnershipTransfer(const Image& image,
                                            uint32_t oldFamilyIndex,
                                            uint32_t newFamilyIndex)
{
  lockResource(image);

  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.srcQueueFamilyIndex = oldFamilyIndex;
  imageBarrier.dstQueueFamilyIndex = newFamilyIndex;
  imageBarrier.image = image.handle();
  imageBarrier.subresourceRange = ImageSlice(image).makeRange();
  imageBarrier.srcAccessMask = 0;
  imageBarrier.dstAccessMask = 0;

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdPipelineBarrier( buffer.handle(),
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        1,
                        &memoryBarrier,
                        0,
                        nullptr,
                        1,
                        &imageBarrier);
}

void CommandProducer::halfOwnershipTransfer(const DataBuffer& buffer,
                                            uint32_t oldFamilyIndex,
                                            uint32_t newFamilyIndex)
{
  lockResource(buffer);

  VkBufferMemoryBarrier bufferBarrier{};
  bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.srcQueueFamilyIndex = oldFamilyIndex;
  bufferBarrier.dstQueueFamilyIndex = newFamilyIndex;
  bufferBarrier.buffer = buffer.handle();
  bufferBarrier.size = buffer.size();
  bufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  bufferBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  CommandBuffer& commandBuffer = getOrCreateBuffer();
  vkCmdPipelineBarrier( commandBuffer.handle(),
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        0,
                        nullptr,
                        1,
                        &bufferBarrier,
                        0,
                        nullptr);
}

CommandBuffer& CommandProducer::getOrCreateBuffer()
{
  // Нельзя создавать новый буфер после финализации
  MT_ASSERT(!_isFinalized);

  if(_currentPrimaryBuffer != nullptr) return *_currentPrimaryBuffer;

  try
  {
    _currentPrimaryBuffer = &_commandPool->getNextBuffer();
    _currentPrimaryBuffer->startOnetimeBuffer();
    _commandSequence.push_back(_currentPrimaryBuffer);
  }
  catch(...)
  {
    _currentPrimaryBuffer = nullptr;
    throw;
  }

  return *_currentPrimaryBuffer;
}

CommandBuffer& CommandProducer::getOrCreatePreparationBuffer()
{
  // Нельзя создавать новый буфер после финализации
  MT_ASSERT(!_isFinalized);

  if (_preparationBuffer != nullptr) return *_preparationBuffer;

  try
  {
    _preparationBuffer = &_commandPool->getNextBuffer();
    _preparationBuffer->startOnetimeBuffer();
    _commandSequence.insert(_commandSequence.begin(), _preparationBuffer);
  }
  catch(...)
  {
    _preparationBuffer = nullptr;
    throw;
  }

  return *_preparationBuffer;
}

void CommandProducer::addImageUsage( const Image& image,
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
                                            ImageAccessWatcher::NEED_MATCHING)
    {
      // Обнаружили место, где будет барьер
      CommandBuffer* matchingBuffer = _cachedMatchingBuffer;
      _cachedMatchingBuffer = nullptr;
      _currentPrimaryBuffer = nullptr;
      matchingBuffer->startOnetimeBuffer();
      _commandSequence.push_back(matchingBuffer);
    }
  }
}

void CommandProducer::addMultipleImagesUsage(MultipleImageUsage usages)
{
  if (_cachedMatchingBuffer == nullptr)
  {
    _cachedMatchingBuffer = &_commandPool->getNextBuffer();
  }

  try
  {
    bool matchingFound = false;

    //  Добавляем информацию об Image с автоконтролем лэйаутов в вотчер лэйаутов
    for(const ImageUsage& usage : usages)
    {
      const Image* image = usage.first;
      if(!image->isLayoutAutoControlEnabled()) continue;

      const ImageAccess& access = usage.second;
      if(_accessWatcher.addImageAccess(
                                    *image,
                                    access,
                                    *_cachedMatchingBuffer) ==
                                              ImageAccessWatcher::NEED_MATCHING)
      {
        matchingFound = true;
      }
    }

    if(matchingFound)
    {
      // Обнаружили место, где будет барьер
      CommandBuffer* matchingBuffer = _cachedMatchingBuffer;
      _cachedMatchingBuffer = nullptr;
      _currentPrimaryBuffer = nullptr;
      matchingBuffer->startOnetimeBuffer();
      _commandSequence.push_back(matchingBuffer);
    }
  }
  catch(std::exception& error)
  {
    // Откатить _accessWatcher посреди списка image-ей не получится, патовая
    // ситуация
    Log::error() << "CommandProducer::_addMultipleImagesUsage: " << error.what();
    MT_ASSERT(false && "Unable to add images usage");
  }
}

void CommandProducer::memoryBarrier(VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags dstStages,
                                    VkAccessFlags srcAccesMask,
                                    VkAccessFlags dstAccesMask)
{
  CommandBuffer& buffer = getOrCreateBuffer();
  buffer.memoryBarrier(srcStages, dstStages, srcAccesMask, dstAccesMask);
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

  lockResource(image);

  CommandBuffer& buffer = getOrCreateBuffer();
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

  lockResource(image);

  ImageAccess imageAccess;
  imageAccess.slices[0] = slice;
  imageAccess.layouts[0] = dstLayout;
  imageAccess.memoryAccess[0] = MemoryAccess{
                                  .readStagesMask = readStages,
                                  .readAccessMask = readAccessMask,
                                  .writeStagesMask = writeStages,
                                  .writeAccessMask = writeAccessMask};
  imageAccess.slicesCount = 1;
  addImageUsage(image, imageAccess);
}
