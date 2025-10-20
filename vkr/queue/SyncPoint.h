#pragma once

#include <util/Assert.h>
#include <vkr/queue/TimelineSemaphore.h>
#include <vkr/Ref.h>

namespace mt
{
  // Пара семафор-значение, которая задает некоторую точку в очереди
  // команд или буфере команд. Используется для синхронизации очередей команд
  // между собой, а также для ожидания на ЦПУ.
  class SyncPoint
  {
  public:
    inline SyncPoint( TimelineSemaphore& semaphore,
                      uint64_t waitingValue) noexcept;
    SyncPoint(const SyncPoint&) noexcept = default;
    SyncPoint& operator = (const SyncPoint&) noexcept = default;
    SyncPoint(SyncPoint&&) noexcept = default;
    SyncPoint& operator = (SyncPoint&&) noexcept = default;
    ~SyncPoint() noexcept = default;

    inline TimelineSemaphore& semaphore() const noexcept;
    inline uint64_t waitingValue() const noexcept;

    // Возвращает true, если синк поинт пройден очередью команд
    inline bool isReady() const;
    // Приостановить работу текущего потока до тех пор, пока синк поинт
    // не будет пройден очередью команд
    inline void waitForReady() const;

  private:
    Ref<TimelineSemaphore> _semaphore;
    uint64_t _waitingValue;
  };

  inline SyncPoint::SyncPoint(TimelineSemaphore& semaphore,
                              uint64_t waitingValue) noexcept :
    _semaphore(&semaphore),
    _waitingValue(waitingValue)
  {
  }

  inline TimelineSemaphore& SyncPoint::semaphore() const noexcept
  {
    return *_semaphore;
  }

  inline uint64_t SyncPoint::waitingValue() const noexcept
  {
    return _waitingValue;
  }

  inline bool SyncPoint::isReady() const
  {
    return _semaphore->getValue() >= _waitingValue;
  }

  inline void SyncPoint::waitForReady() const
  {
    _semaphore->waitFor(_waitingValue);
  }
}