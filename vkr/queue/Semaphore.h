#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

namespace mt
{
  class Device;

  // Обертка вокруг VkSemaphore, реализующая семафор с двумя сотояниями "signal"
  // и "unsignal" (не timeline semaphore).
  // Примитив синхронизации очередей на стороне GPU
  // Вместо этого класса лучше использовать синхронизацию через
  // TimelineSemaphore и SyncPoint. Класс оставлен в основном ради синхронизации
  // со свапчейном.
  class Semaphore : public RefCounter
  {
  public:
    explicit Semaphore(Device& device);
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator = (const Semaphore&) = delete;
  protected:
    virtual ~Semaphore() noexcept;

  public:
    inline VkSemaphore handle() const noexcept;
    inline Device& device() const noexcept;

  private:
    void _clear() noexcept;

  private:
    Device& _device;
    VkSemaphore _handle;
  };

  inline VkSemaphore Semaphore::handle() const noexcept
  {
    return _handle;
  }

  inline Device& Semaphore::device() const noexcept
  {
    return _device;
  }
}