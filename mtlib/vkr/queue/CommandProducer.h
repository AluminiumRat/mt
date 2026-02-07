#pragma once

#include <optional>
#include <span>
#include <utility>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <vkr/image/ImageAccessWatcher.h>
#include <vkr/queue/CommandPool.h>
#include <vkr/queue/UniformMemoryPool.h>

namespace mt
{
  class CommandBuffer;
  class CommandPoolSet;
  class CommandQueue;
  class ImageSlice;
  class DataBuffer;
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
  //  Включает в себя только базовый функционал взаимодействия с очередью команд
  //    и набор общих утилит. За реализацию конкретного рабочего функционала
  //    отвесают классы CommandProducerTransfer, CommandProducerCompute и
  //    CommandProducerGraphic, которые унаследованы от него.
  class CommandProducer
  {
  public:
    using ImageUsage = std::pair<const Image*, ImageAccess>;
    using MultipleImageUsage = std::span<const ImageUsage>;

  public:
    //  Если debugName - не пустая строка, то все команды продюсера
    //  будут обернуты в vkCmdBeginDebugUtilsLabelEXT и
    //  vkCmdEndDebugUtilsLabelEXT
    CommandProducer(CommandPoolSet& poolSet, const char* debugName);
    CommandProducer(const CommandProducer&) = delete;
    CommandProducer& operator = (const CommandProducer&) = delete;
    virtual ~CommandProducer() noexcept;

    inline CommandQueue& queue() const noexcept;

    inline VolatileDescriptorPool& descriptorPool() noexcept;
    inline UniformMemoryPool::Session& uniformMemorySession() noexcept;

    //  Барьер памяти. То есть разделяем поток исполнения команд плюс
    //    флашим кэши srcAccesMask и инвалидируем кэши dstAccesMask
    void memoryBarrier( VkPipelineStageFlags srcStages,
                        VkPipelineStageFlags dstStages,
                        VkAccessFlags srcAccesMask,
                        VkAccessFlags dstAccesMask);

    //  Перевод имэйджа из одного лайоута в другой.
    //  Можно использовать только на имэйджах с отключенным автоконтролем
    //    лэйаута.
    void imageBarrier(const Image& image,
                      const ImageSlice& slice,
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
    void forceLayout( const Image& image,
                      const ImageSlice& slice,
                      VkImageLayout dstLayout,
                      VkPipelineStageFlags readStages,
                      VkAccessFlags readAccessMask,
                      VkPipelineStageFlags writeStages,
                      VkAccessFlags writeAccessMask);

    //  Получить буфер команд, предназначенный для основных выполняемых операций
    CommandBuffer& getOrCreateBuffer();
    //  Получить буфер для подготовительных команд перед рендер пассом. Если
    //    продюсер в данный момент записывает рендер пасс, то будет возвращен
    //    буфер, находящийся перед текущим. Если рендер пасс не записывается,
    //    то будет возвращен текущий буфер.
    //  Метод необходим для проведения подготовительных работ перед проходом
    //    рендера, так как во время прохода запрещено менять лэйауты image-ей и
    //    переносить(копировать) данные
    CommandBuffer& getPreparationBuffer();

    //  Зарегистрировать использование Image. Работает для поддержки
    //  автоконтроля лэйаутов
    void addImageUsage(const Image& image, const ImageAccess& access);
    //  Зарегистрировать использование нескольких Image. Работает для поддержки
    //    автоконтроля лэйаутов.
    void addMultipleImagesUsage(MultipleImageUsage usages);

    //  Захватить владение ресурсом. Это продляет жизнь ресурса и позволяет
    //  предотвратить его удаление, пока буферы команд находятся на исполнении в
    //  очереди команд
    inline void lockResource(const RefCounter& resource);

    //  Начать именованную область команд. Нужно для профайлеров и RenderDoc-а,
    //    чтобы разметить кадр по стадиям.
    //  Работает только при включенном дебаге в VKRLib. При выключенном дебаге
    //    ничего не делает
    void beginDebugLabel(const char* label);
    //  Начать именованную область команд.
    //  Работает только при включенном дебаге в VKRLib.
    void endDebugLabel();

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
      //  Пулл, из которого можно создать буффер для согласования Image
      //    layout-ов
      //  Всегда не nullptr
      CommandPool* commandPool = nullptr;

      //  Информация об Image-ах с автоконтролем Layout-ов
      //  Всегда не nullptr
      const ImageAccessMap* imageStates;
    };

    //  Завершить запись команд. Здесь происходит сброс на ГПУ данных
    //    uiform буферов и финализация буферов команд.
    //  После вызова finalize использовать CommanProducer для записи команд
    //    уже нельзя.
    //  Если не было записано ни одной команды, то вернет nullopt
    virtual std::optional<FinalizeResult> finalize() noexcept;
    //  Отправить пулы на передержку до достижения releasePoint
    void release(const SyncPoint& releasePoint);

    //  Половина трансфера владения (vulkan Queue Family Ownership Transfer)
    //  Команды, исполняемые на каждой из очередей, участвующих в трансфере
    void halfOwnershipTransfer( const Image& image,
                                uint32_t oldFamilyIndex,
                                uint32_t newFamilyIndex);

    //  Половина трансфера владения (vulkan Queue Family Ownership Transfer)
    //  Команды, исполняемые на каждой из очередей, участвующих в трансфере
    void halfOwnershipTransfer( const DataBuffer& buffer,
                                uint32_t oldFamilyIndex,
                                uint32_t newFamilyIndex);

  protected:
    //  Начинает блок, внутри которого нельзя менять лэйауты Image-ей, менять
    //  текущий буфер команд и производить копирование(трансфер) данных
    void beginRenderPassBlock();
    void endRenderPassBlock() noexcept;
    inline bool insideRenderPass() const noexcept;

  protected:
    //  Вызывается при финализации продюсера. Предназначен для классов-потомков,
    //  чтобы они могли корректно завершать запись команд.
    virtual void finalizeCommands() noexcept;

  private:
    //  Найти или создать буфер, в который можно вставить Image барьеры
    CommandBuffer* _getMatchingBuffer();

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
    CommandBuffer* _currentBuffer;

    //  Флаг, который говорит о том, что вданный момент формируются команды для
    //  рендер пасса
    bool _insideRenderPass;

    //  Для дебажных целей
    std::string _debugName;

    bool _isFinalized;
  };

  inline CommandQueue& CommandProducer::queue() const noexcept
  {
    return _queue;
  }

  inline VolatileDescriptorPool& CommandProducer::descriptorPool() noexcept
  {
    MT_ASSERT(_descriptorPool != nullptr);
    return *_descriptorPool;
  }

  inline UniformMemoryPool::Session&
                                CommandProducer::uniformMemorySession() noexcept
  {
    MT_ASSERT(_uniformMemorySession.has_value());
    return _uniformMemorySession.value();
  }

  inline void CommandProducer::lockResource(const RefCounter& resource)
  {
    MT_ASSERT(_commandPool != nullptr);
    _commandPool->lockResource(resource);
  }

  inline bool CommandProducer::insideRenderPass() const noexcept
  {
    return _insideRenderPass;
  }
}