#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandPoolSet.h>
#include <vkr/queue/CommandQueue.h>

using namespace mt;

CommandPoolSet::CommandPoolSet(CommandQueue& commandQueue) :
  _queue(commandQueue)
{
}

CommandPool& CommandPoolSet::getPool()
{
  // Для начала ищем, не освободился ли кто-то из ожидающих пулов
  for(WaitingPools::iterator iPool = _waitingPools.begin();
      iPool != _waitingPools.end();
      iPool++)
  {
    if(iPool->releasePoint.isReady())
    {
      CommandPool* pool = iPool->pool;
      pool->reset();
      _waitingPools.erase(iPool);
      return *pool;
    }
  }

  // Свободных пулов нет, создаем новый
  std::unique_ptr<CommandPool> newPool(new CommandPool(_queue));
  CommandPool& newPoolRef = *newPool;
  _pools.push_back(std::move(newPool));
  return newPoolRef;
}

void CommandPoolSet::returnPool(CommandPool& pool,
                                const SyncPoint& releasePoint) noexcept
{
  try
  {
    // Для начала проверим, что пул был создан этим сетом
    bool poolFound = false;
    for(std::unique_ptr<CommandPool>& myPool : _pools)
    {
      if(myPool.get() == &pool)
      {
        poolFound = true;
        break;
      }
    }
    MT_ASSERT(poolFound);

    // Теперь проверим, что пула ещё нет в списке ожидания
    for(WaitingPool& waitingPool : _waitingPools)
    {
      MT_ASSERT(waitingPool.pool != &pool);
    }

    // Всё хорошо, пул можно добавлять в лист ожидания
    _waitingPools.push_back(WaitingPool{.pool = &pool,
                                        .releasePoint = releasePoint});
  }
  catch (std::runtime_error& exception)
  {
    Log::error() << "CommandPoolSet::returnPool: " << exception.what();
    MT_ASSERT(false && "Unable to return Command pool to CommandPoolSet");
  }
}
