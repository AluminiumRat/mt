#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>
#include <vkr/DescriptorCounter.h>

namespace mt
{
  class DescriptorSetLayout;
  class Device;

  //  Пул фиксированной величины, содержащий дескриптеры ресурсов для пайплайна.
  //    Фактически - обертка вокруг VkDescriptorPool, контролирующая расход
  //    дескриптеров и сетов из пула.
  //  Вы можете выделять из пула дескриптор сеты, пока не израсходуете
  //    запас дескрипторов или сетов. Метод DescriptorPool::reset уничтожает
  //    все выделенные из пула сеты и освобождает (возвращает в пул) все
  //    использованные дескрипторы.
  class DescriptorPool : public RefCounter
  {
  public:
    enum Mode
    {
      //  Дескриптер сеты выделяются один раз и переиспользуются из кадра в
      //    кадр. Вернуть и переиспользовать дескриптеры невозможно.
      //  В этом режиме нельзя использовать методы allocateVolatileSet и
      //    reset.
      STATIC_POOL,
      //  Режим предназначен для использования внутри VolatileDescriptorPool
      //  Дескриптер сеты можно вернуть в пул методом reset, однако необходимо
      //    следить за тем, что дескриптеры не используются в очереди команд
      //    на момет вызова reset
      //  В этом режиме нельзя использовать метод allocateStaticSet
      VOLATILE_POOL
    };

  public:
    DescriptorPool( Device& device,
                    const DescriptorCounter& totalDescriptors,
                    uint32_t totalSets,
                    Mode mode);
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator = (const DescriptorPool&) = delete;
  protected:
    ~DescriptorPool() noexcept;

  public:
    inline VkDescriptorPool handle() const;
    inline Device& device() const;

    inline Mode mode() const noexcept;

    inline const DescriptorCounter& totalDescriptors() const;
    inline const DescriptorCounter& descriptorsLeft() const;

    inline uint32_t totalSets() const;
    inline uint32_t setsLeft() const;
    inline uint32_t setsAllocated() const;

    //  Выделение дескриптор сетя в режиме VOLATILE_POOL
    //  Выделенные сеты могут быть возвращены в пул с помощью метода reset
    VkDescriptorSet allocateVolatileSet(const DescriptorSetLayout& layout);
    //  Вернуть все выделенные сеты обратно в пул.
    //  Может быть вызван только в режиме VOLATILE_POOL
    //  ВНИМАНИЕ! Необходимо гарантировать, что на момент вызова reset ни один
    //    из сетов не используется ни в одной из очередей команд.
    void reset();

  private:
    void _cleanup() noexcept;

  private:
    VkDescriptorPool _handle;
    Device& _device;

    Mode _mode;

    DescriptorCounter _totalDescriptors;
    DescriptorCounter _descriptorsLeft;

    uint32_t _totalSets;
    uint32_t _setsLeft;
  };

  inline VkDescriptorPool DescriptorPool::handle() const
  {
    return _handle;
  }

  inline Device& DescriptorPool::device() const
  {
    return _device;
  }

  inline DescriptorPool::Mode DescriptorPool::mode() const noexcept
  {
    return _mode;
  }

  inline const DescriptorCounter& DescriptorPool::totalDescriptors() const
  {
    return _totalDescriptors;
  }

  inline const DescriptorCounter& DescriptorPool::descriptorsLeft() const
  {
    return _descriptorsLeft;
  }

  inline uint32_t DescriptorPool::totalSets() const
  {
    return _totalSets;
  }

  inline uint32_t DescriptorPool::setsLeft() const
  {
    return _setsLeft;
  }

  inline uint32_t DescriptorPool::setsAllocated() const
  {
    return _totalSets - _setsLeft;
  }
}