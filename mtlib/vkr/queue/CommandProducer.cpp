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
#include <vkr/VKRLib.h>

using namespace mt;

CommandProducer::CommandProducer( CommandPoolSet& poolSet,
                                  const char* debugName) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _currentBuffer(nullptr),
  _descriptorPool(nullptr),
  _insideRenderPass(false),
  _debugName(debugName),
  _isFinalized(false)
{
  try
  {
    _commandPool = &_commandPoolSet.getPool();
    _descriptorPool = &_commandPool->descriptorPool();
    _uniformMemorySession.emplace(_commandPool->memoryPool());
    _commandSequence.reserve(1024);

    if(!_debugName.empty()) beginDebugLabel(_debugName.c_str());
  }
  catch (std::exception& error)
  {
    Log::error() << "CommandProducer: " << error.what();
    MT_ASSERT(false && "Unable to create CommandProducer");
  }
}

void CommandProducer::finalizeCommands() noexcept
{
  //  Закрываем дебажную информацию в очереди команд
  if(!_debugName.empty()) endDebugLabel();
}

std::optional<CommandProducer::FinalizeResult>
                                          CommandProducer::finalize() noexcept
{
  MT_ASSERT(!_isFinalized);
  MT_ASSERT(!_insideRenderPass);

  finalizeCommands();

  try
  {
    _isFinalized = true;

    _uniformMemorySession.reset();

    if(_commandSequence.empty()) return std::nullopt;

    FinalizeResult result;
    result.commandSequence = &_commandSequence;
    result.commandPool = _commandPool;
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

  if(_currentBuffer != nullptr) return *_currentBuffer;

  try
  {
    _currentBuffer = &_commandPool->getNextBuffer();
    _currentBuffer->startOnetimeBuffer();
    _commandSequence.push_back(_currentBuffer);
  }
  catch(...)
  {
    _currentBuffer = nullptr;
    throw;
  }

  return *_currentBuffer;
}

void CommandProducer::beginRenderPassBlock()
{
  MT_ASSERT(!_insideRenderPass);

  //  Создаем новый буфер команд для того чтобы не поймать конфликты лэйаутов
  //  с командами, которые уже записаны в текущем буфере
  _currentBuffer = nullptr;
  getOrCreateBuffer();

  _insideRenderPass = true;
}

void CommandProducer::endRenderPassBlock() noexcept
{
  MT_ASSERT(_insideRenderPass);
  _insideRenderPass = false;
}

CommandBuffer& CommandProducer::getPreparationBuffer()
{
  MT_ASSERT(!_isFinalized);

  if(!_insideRenderPass)
  {
    //  Если сейчас не заполняется рендер пасс, то вспомогательные команды можно
    //  записывать в конец последнего буфера
    if(_commandSequence.empty()) return getOrCreateBuffer();
    return *_commandSequence.back();
  }
  else
  {
    //  Если мы записываем рендер пасс, то барьеры нужно вставлять до текущего
    //  буфера, так как текущий буфер нельзя разрывать (начало и конец рендер
    //  пасса должны быть в одном буфере)
    MT_ASSERT(!_commandSequence.empty());
    if(_commandSequence.size() > 1)
    {
      return *_commandSequence[_commandSequence.size() - 2];
    }
    else
    {
      //  Если в списке буферов есть только один единственный буфер, значит
      //  это буфер, созданный для рендер пасса, нужно добавить ещё один перед
      //  ним, и в него уже писать барьеры
      CommandBuffer& matchingBuffer = _commandPool->getNextBuffer();
      matchingBuffer.startOnetimeBuffer();
      _commandSequence.insert(_commandSequence.begin(), &matchingBuffer);
      return matchingBuffer;
    }
  }
}

void CommandProducer::addImageUsage( const Image& image,
                                      const ImageAccess& access)
{
  if(!image.isLayoutAutoControlEnabled()) return;

  CommandBuffer& matchingBuffer = getPreparationBuffer();
  if(_accessWatcher.addImageAccess( image,
                                    access,
                                    matchingBuffer) ==
                                              ImageAccessWatcher::NEED_MATCHING)
  {
    //  Если нет рендер пасса, то текущий буфер необходимо завершить, так как
    //  _accessWatcher будет дописывать в него барьеры, но будет делать это
    //  позже
    if(!_insideRenderPass) _currentBuffer = nullptr;
  }
}

void CommandProducer::addMultipleImagesUsage(MultipleImageUsage usages)
{
  if(usages.empty()) return;

  CommandBuffer& matchingBuffer = getPreparationBuffer();

  try
  {
    for(const ImageUsage& usage : usages)
    {
      const Image* image = usage.first;
      if(!image->isLayoutAutoControlEnabled()) continue;

      const ImageAccess& access = usage.second;
      if(_accessWatcher.addImageAccess(
                                    *image,
                                    access,
                                    matchingBuffer) ==
                                              ImageAccessWatcher::NEED_MATCHING)
      {
        //  Если нет рендер пасса, то текущий буфер необходимо завершить, так
        //  как _accessWatcher будет дописывать в него барьеры, но будет
        //  делать это позже
        if(!_insideRenderPass) _currentBuffer = nullptr;
      }
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
  MT_ASSERT(!insideRenderPass());
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

void CommandProducer::beginDebugLabel(const char* label)
{
  if (!VKRLib::instance().isDebugEnabled()) return;

  VkDebugUtilsLabelEXT labelInfo{};
  labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  labelInfo.pLabelName = label;

  CommandBuffer& buffer = getOrCreateBuffer();
  Device& device = _queue.device();
  device.extFunctions().vkCmdBeginDebugUtilsLabelEXT( buffer.handle(),
                                                      &labelInfo);
}

void CommandProducer::endDebugLabel()
{
  if (!VKRLib::instance().isDebugEnabled()) return;

  CommandBuffer& buffer = getOrCreateBuffer();
  Device& device = _queue.device();
  device.extFunctions().vkCmdEndDebugUtilsLabelEXT(buffer.handle());
}