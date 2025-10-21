#pragma once

#include <vulkan/vulkan.h>

#include <vkr/DescriptorCounter.h>
#include <vkr/RefCounter.h>

namespace mt
{
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
    DescriptorPool( const DescriptorCounter& totalDescriptors,
                    uint32_t totalSets,
                    Device& device);
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator = (const DescriptorPool&) = delete;
  protected:
    ~DescriptorPool() noexcept;

  public:
    inline VkDescriptorPool handle() const;
    inline Device& device() const;

    inline const DescriptorCounter& totalDescriptors() const;
    inline const DescriptorCounter& descriptorsLeft() const;

    /// Result descriptor set will be returned to the pool in the next frame via
    /// a call of the reset method.
    VkDescriptorSet allocateSet(VkDescriptorSetLayout layout,
                                const DescriptorCounter& descriptorsNumber);

    inline uint32_t totalSets() const;
    inline uint32_t setsLeft() const;
    inline uint32_t setsAllocated() const;

    void reset();

  private:
    void _cleanup() noexcept;

  private:
    VkDescriptorPool _handle;
    Device& _device;

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