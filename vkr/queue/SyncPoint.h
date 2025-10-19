#pragma once

#include <util/Assert.h>
#include <vkr/queue/TimelineSemaphore.h>
#include <vkr/Ref.h>

namespace mt
{
  // Пара семафор-значение, которая задает некоторую точку в очереди
  // команд или буфере команд. Вы можете сделать ожидание этой точки как со
  // стороны GPU, так и со стороны CPU
  struct SyncPoint
  {
    Ref<TimelineSemaphore> semaphore;
    uint64_t semaphoreValue;
  };
}