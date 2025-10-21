#pragma once

#include <memory>
#include <vector>

#include <vkr/queue/CommandPool.h>
#include <vkr/queue/SyncPoint.h>

namespace mt
{
  class CommandQueue;

  //  Пул пулов команд. Передержка для пулов команд, чтобы они могли дождаться,
  //    когда все их команды будут выполнены. Необходим для того чтобы
  //    обеспечить безопасный ресет пула в момент, когда очередь команд его не
  //    использует.
  //  Отслеживает, можно ли ресетить пул, по синк поинтам очереди команд.
  class CommandPoolSet
  {
  public:
    CommandPoolSet(CommandQueue& commandQueue);
    CommandPoolSet(const CommandPoolSet&) = delete;
    CommandPoolSet& operator = (const CommandPoolSet&) = delete;
    ~CommandPoolSet() noexcept = default;

    CommandPool& getPool();

    //  Вернуть пул обратно в сет на передержку.
    //  releasePoint - точка в очереди команд, когда пул можно
    //    переиспользовать.
    //  Возвращать можно только те пулы, которые были созданы этим сетом.
    //  На один getPool должно приходиться не больше одного returnPool
    void returnPool(CommandPool& pool, const SyncPoint& releasePoint);

  private:
    CommandQueue& _queue;

    // Все созданные в этом сете пулы. Сохраняем для удаления вместе с сетом.
    std::vector<std::unique_ptr<CommandPool>> _pools;

    struct WaitingPool
    {
      CommandPool* pool;
      SyncPoint releasePoint;
    };
    using WaitingPools = std::vector<WaitingPool>;
    // Пулы, находящиеся на передержке и ждущие, когда очередь команд закончит
    // оаботу с ними
    WaitingPools _waitingPools;
  };
}