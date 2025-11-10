#pragma once

#include <memory>
#include <vector>

#include <util/Ref.h>
#include <vkr/DescriptorPool.h>

namespace mt
{
  class DescriptorSetLayout;
  class Device;

  //  Пул дескрипторов, специально предназначенный для создания сетов "на лету"
  //    во время построения буферов команд (для ресурсов, которые постоянно
  //    меняются от кадра к кадру).
  //  Пул масштабируемый, то есть при необходимости увеличивает свой размер.
  //    По факту - просто набор объектов DescriptorPool
  //  Предполагает сессионное использование. В начале сессии вызывается метод
  //    VolatileDescriptorPool::reset, который уничтожает все выделенные
  //    дескриптер сеты и возвращает дескриптеры в пул. В течении сессии
  //    из пула выделяются сеты, которые используются для подключения
  //    ресурсов к пайплайнам. Для начала следующей сессии необходимо вызвать
  //    reset, но не раньше, чем прекратится использование выделенных сетов
  //    в очередях команд.
  //  ВНИМАНИЕ! VolatileDescriptorPool оставляет контроль за временем жизни
  //    пулов и дескриптеров на стороне пользователя. Перед удалением
  //    пула убедитесь, что ни одна очередь не использует сеты, выделенные
  //    из него.
  class VolatileDescriptorPool
  {
  public:
    VolatileDescriptorPool( const DescriptorCounter& initialDescriptorNumber,
                            uint32_t initialSetNumber,
                            Device& device);
    VolatileDescriptorPool(const VolatileDescriptorPool&) = delete;
    VolatileDescriptorPool& operator =
                                    (const VolatileDescriptorPool&) = delete;
    ~VolatileDescriptorPool() noexcept = default;

    VkDescriptorSet allocateSet(const DescriptorSetLayout& layout);

    //  Уничтожить все выделенные из пула сеты и вернуть в пул все дескриптеры.
    //  ВНИМАНИЕ! Убедитесь, что выделенные сеты не используются ни в одной
    //    из очередей команд.
    void reset();

  private:
    void _addPool();
    void _selectPool(size_t poolIndex);

  private:
    Device& _device;
    DescriptorCounter _initialDescriptorNumber;
    uint32_t _initialSetNumber;

    using Subpools = std::vector<Ref<DescriptorPool>>;
    Subpools _subpools;
    size_t _currentPoolIndex;
    DescriptorPool* _currentPool;
  };
}