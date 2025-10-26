#pragma once

#include <optional>

#include <vulkan/vulkan.h>

#include <vkr/queue/ImageAccessWatcher.h>
#include <vkr/queue/UniformMemoryPool.h>

namespace mt
{
  class CommandBuffer;
  class CommandPool;
  class CommandPoolSet;
  class CommandQueue;
  class ImageSlice;
  class SyncPoint;
  class VolatileDescriptorPool;

  //  Класс, отвечающий за корректное наполнение буферов команд и отправку их
  //    в очередь команд. Куча рутины по сшиванию в единое целое буферов,
  //    очередей, пулов и ресурсов.
  //  Класс предполагает одноразовое использование и реализует механизм сессий
  //    для пулов. То есть в конструкторе происходит поиск свободных пулов и
  //    старт сессий для них. Окончание сессий и отправка команд в очередь
  //    происходит при передаче продюсера обратно в соответствующую очередь
  //    команд.
  //  Уничтожение продюсера команд вне очереди команд не жедательно,
  //    так как может создать гонки при многопоточном использовании
  //    нескольких продюсеров от одной очереди. Кроме того, хотя сессии пулов
  //    и будут завершены, но собранный буфер команд не будет отправлем на
  //    выполнение.
  class CommandProducer
  {
  public:
    CommandProducer(CommandPoolSet& poolSet);
    CommandProducer(const CommandProducer&) = delete;
    CommandProducer& operator = (const CommandProducer&) = delete;
    ~CommandProducer() noexcept;

    inline CommandQueue& queue() const noexcept;

    //  Перевод имэйджа из одного лайоута в другой.
    //  Можно использовать только на имэйджах с отключенным автоконтролем
    //    лэйаута.
    void imageBarrier(const ImageSlice& slice,
                      VkImageLayout srcLayout,
                      VkImageLayout dstLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags dstStages,
                      VkAccessFlags srcAccesMask,
                      VkAccessFlags dstAccesMask);

    //  Зафорсить перевод Image-а в конкретный лэйаут
    //  Можно использовать только на Image-ах со включенным автоконтролем
    //    лэйаута
    //  readStages, readAccessMask, writeStages, writeAccessMask определяют,
    //    на каких стадиях и через какие кэши будет происходить чтение и запись
    //    после смены лэйаута.
    void forceLayout( const ImageSlice& slice,
                      VkImageLayout dstLayout,
                      VkPipelineStageFlags readStages,
                      VkAccessFlags readAccessMask,
                      VkPipelineStageFlags writeStages,
                      VkAccessFlags writeAccessMask);

  private:
    //  Отправка буфера на выполнение и релиз продюсера должны выполняться
    //  под мьютексом очередей команд, поэтому доступ только из очереди команд
    friend class CommandQueue;

    struct FinalizeResult
    {
      //  Буферы, в которые были записаны команды во время работы продюсера
      //  Всегда не nullptr, и всегда в завершенном состоянии, то есть с уже
      //    вызванным CommandBuffer::endBuffer
      const std::vector<CommandBuffer*>* commandSequence;
      //  Пустой буфер команд, предназначенный для согласования предыдущих
      //    буферов с только что заполненными. То есть он предназначен для
      //    барьеров.
      //  Всегда не nullptr
      //  Буфер возвращается в нестартованном состоянии, то есть для него
      //    необходимо вызвать CommandBuffer::startOnetimeBuffer.
      CommandBuffer* approvingBuffer = nullptr;

      //  Информация об Image-ах с автоконтролем Layout-ов
      //  Всегда не nullptr
      const ImageAccessMap* imageStates;
    };

    //  Завершить запись команд. Здесь происходит сброс на ГПУ данных
    //    uiform буферов и финализация буферов команд.
    //  После вызова finalize использовать CommanProducer для записи команд
    //    уже нельзя.
    //  Если не было записано ни одной команды, то вернет nullopt
    std::optional<FinalizeResult> finalize() noexcept;
    //  Отправить пулы на передержку до достижения releasePoint
    void release(const SyncPoint& releasePoint);

  private:
    CommandBuffer& _getOrCreateBuffer();
    void _addImageUsage(const SliceAccess& sliceAccess);

  private:
    CommandPoolSet& _commandPoolSet;
    CommandQueue& _queue;
    CommandPool* _commandPool;
    VolatileDescriptorPool* _descriptorPool;
    std::optional<UniformMemoryPool::Session> _uniformMemorySession;
    ImageAccessWatcher _accessWatcher;

    //  Буферы команд в том порядке, в котором их необходимо добавлять
    //  в очередь
    std::vector<CommandBuffer*> _commandSequence;
    //  Текущий заполняемый буфер команд
    CommandBuffer* _currentPrimaryBuffer;
    //  Зарезервированный, но ещё не, использованный буфер для согласования
    CommandBuffer* _currentApprovingBuffer;

    bool _isFinalized;
  };

  inline CommandQueue& CommandProducer::queue() const noexcept
  {
    return _queue;
  }
}