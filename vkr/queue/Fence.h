#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

namespace mt
{
  class Device;

  // Обертка вокруг VkFence
  // Примитив синхронизации между CPU и GPU. Позволяет приостанавливать
  // поток CPU до того момента, как со стороны GPU для фэнса не будет выставлено
  // состояние "signaled"
  // Вместо этого класса лучше использовать синхронизацию через
  // TimelineSemaphore и SyncPoint. Класс оставлен в основном ради синхронизации
  // со свапчейном.
  class Fence : public RefCounter
  {
  public:
    explicit Fence(Device& device);
    Fence(const Fence&) = delete;
    Fence& operator = (const Fence&) = delete;
  protected:
    virtual ~Fence() noexcept;

  public:
    inline VkFence handle() const noexcept;
    inline Device& device() const noexcept;

    // Ждать, пока фэнс не выставит сигнал со стороны GPU
    // timeout в наносекундах
    void wait(uint64_t timeout = UINT64_MAX);

    // Сбросить фэнс в состояние "unsignal"
    void reset();

  private:
    void _clear() noexcept;

  private:
    Device& _device;
    VkFence _handle;
  };

  inline VkFence Fence::handle() const noexcept
  {
    return _handle;
  }

  inline Device& Fence::device() const noexcept
  {
    return _device;
  }
}