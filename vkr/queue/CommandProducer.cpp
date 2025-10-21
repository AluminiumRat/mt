#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandPool.h>
#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandProducer.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/VolatileDescriptorPool.h>

using namespace mt;

CommandProducer::CommandProducer(CommandPoolSet& poolSet) :
  _commandPoolSet(poolSet),
  _queue(_commandPoolSet.queue()),
  _commandPool(nullptr),
  _commandBuffer(nullptr),
  _descriptorPool(nullptr)
{
}

void CommandProducer::flushUniformData()
{
  if(_uniformMemorySession.has_value())
  {
    _uniformMemorySession.reset();
  }
}

void CommandProducer::release(const SyncPoint& releasePoint)
{
  flushUniformData();

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

CommandBuffer& CommandProducer::getOrCreateBuffer()
{
  if(_commandBuffer != nullptr) return *_commandBuffer;

  if(_commandPool != nullptr) _commandPool = &_commandPoolSet.getPool();
  if(_descriptorPool != nullptr)
  {
    _descriptorPool = &_commandPool->descriptorPool();
  }

  _uniformMemorySession.emplace(_commandPool->memoryPool());
  _commandBuffer = &_commandPool->getNextBuffer();

  return *_commandBuffer;
}
