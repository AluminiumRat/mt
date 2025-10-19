#pragma once

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;

  // Обертка вокруг VkSemaphore, реализующая таймлайн семафор, то есть семафор со
  // счетчиком.
  // На базе этого класса построены точки синхронизации (класс SyncPoint)
  // Таймлайн семафоры поддерживают синхронизацию не только на стороне ГПУ, но
  // и синхронизацию ЦПУ-ГПУ
  class TimelineSemaphore : public RefCounter
  {
  public:
    TimelineSemaphore(uint64_t initialValue, Device& device);
    TimelineSemaphore(const TimelineSemaphore&) = delete;
    TimelineSemaphore& operator = (const TimelineSemaphore&) = delete;
  protected:
    virtual ~TimelineSemaphore() noexcept;

  public:
    inline Device& device() const noexcept;

    // Получить текущее актуальное значение семафора
    uint64_t getValue() const;

    // Приостановить работу текущего потока до тех пор, пока семафор не примет
    // значение равно или больше value
    void waitFor(uint64_t value) const;

  private:
    // Запрещаем прямые манипуляции с хэндлм семафора, чтобы не поломать
    // механизм синхронизации очередей
    friend class CommandQueue;
    inline VkSemaphore handle() const noexcept;

  private:
    void _clear() noexcept;

  private:
    Device& _device;
    VkSemaphore _handle;
  };

  inline VkSemaphore TimelineSemaphore::handle() const noexcept
  {
    return _handle;
  }

  inline Device& TimelineSemaphore::device() const noexcept
  {
    return _device;
  }
}