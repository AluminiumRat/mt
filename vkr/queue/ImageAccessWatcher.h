#pragma once

#include <vulkan/vulkan.h>

#include <vkr/queue/ImageAccess.h>

namespace mt
{
  class CommandBuffer;

  //  Класс, который во время работы CommandProducer-а отслеживает доступ к
  //  Image-ам с автоконтролем layout-ов, и формирует барьеры для переходов
  //  между лэйаутами.
  class ImageAccessWatcher
  {
  public:
    // Случится ли конфликт по доступу к памяти
    enum MemoryConflict
    {
      NO_MEMORY_CONFLICT,
      NEED_TO_APPROVE
    };

  public:
    ImageAccessWatcher() noexcept;
    ImageAccessWatcher(const ImageAccessWatcher&) = delete;
    ImageAccessWatcher& operator = (const ImageAccessWatcher&) = delete;
    ~ImageAccessWatcher() noexcept = default;

    //  Добавить информацию об использовании Image-а и создать барьеры при
    //    необходимости.
    //  Если возвращает NEED_TO_APPROVE, то approvingBuffer будет
    //    сохранен для использования в качестве точки
    //    согласования (ApprovingPoint), и должен быть добавлен в очередь команд
    //    до описываемого использования Image-а. Непосредственно во время этого
    //    вызова буфер не будет заполнен, так требования по согласованию могут
    //    ужесточаться по мере добавления новых команд.
    //  Если возвращает NO_MEMORY_CONFLICT, то approvingBuffer в вызове не
    //    использовался, и он можеть быть переиспользован для чего-то другого.
    //  approvingBuffer непосредственно в вызове заполняться не будет, поэтому
    //    его можно стартовать только когда будет возвращен NEED_TO_APPROVE
    MemoryConflict addImageAccess(const SliceAccess& sliceAccess,
                                  CommandBuffer& approvingBuffer) noexcept;

    //  Закончить работу, записать все необходимые барьеры и привести
    //    ImageAccessMap в вид, пригодный для внешнего использования.
    //  ВНИМАНИЕ! После финализации добавлять новую информацию о доступе
    //    нельзя.
    const ImageAccessMap& finalize() noexcept;

  private:
    void _addApprovingPoint(const Image& image,
                            ImageAccessLayoutState& imageState,
                            CommandBuffer& approvingBuffer,
                            const LayoutTranslation& translation,
                            const ResourceAccess& newMemoryAccess);

  private:
    ImageAccessMap _accessMap;
    bool _isFinalized;
  };
}