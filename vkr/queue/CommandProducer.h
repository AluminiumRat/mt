#pragma once

#include <memory>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include <vkr/queue/ImageLayoutState.h>
#include <vkr/queue/UniformMemoryPool.h>
#include <vkr/ImageSlice.h>

namespace mt
{
  class CommandBuffer;
  class CommandPool;
  class CommandPoolSet;
  class CommandQueue;
  class Image;
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

    //  Перевод имэйджа из одного лайоута в другой. Можно использовать
    //    только на имэйджах с отключенным автоконтролем лэйаута.
    void imageBarrier(const ImageSlice& slice,
                      VkImageLayout srcLayout,
                      VkImageLayout dstLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags dstStages,
                      VkAccessFlags srcAccesMask,
                      VkAccessFlags dstAccesMask);

  private:
    //  Отправка буфера на выполнение и релиз продюсера должны выполняться
    //  под мьютексом очередей команд, поэжтому доступ только из очереди
    friend class CommandQueue;
    //  Сбросить на GPU все данные для юниформ буферов перед отправкой
    //  в очередь команд и финализировать текущий буфер команд.
    void finalize();
    inline CommandBuffer* commandBuffer() const noexcept;
    //  Отправить пулы на передержку до достижения releasePoint
    void release(const SyncPoint& releasePoint);

  private:
    CommandBuffer& _getOrCreateBuffer();

    //  Сделать все необходимые отметки и настройки перед использованием
    //  Image в буфере команд.
    void _addImageUsage(const ImageSlice& slice,
                        VkImageLayout requiredLayout,
                        VkPipelineStageFlags readStagesMask,
                        VkAccessFlags readAccessMask,
                        VkPipelineStageFlags writeStagesMask,
                        VkAccessFlags writeAccessMask);

    //  Перевести слайс в нужный layout и сделать отметку об этом в imageState
    void _addImageLayoutTransform(const ImageSlice& slice,
                                  ImageLayoutState& imageState,
                                  VkImageLayout requiredLayout,
                                  VkPipelineStageFlags readStagesMask,
                                  VkAccessFlags readAccessMask,
                                  VkPipelineStageFlags writeStagesMask,
                                  VkAccessFlags writeAccessMask);

  private:
    CommandPoolSet& _commandPoolSet;
    CommandQueue& _queue;
    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;
    bool _bufferInProcess;
    VolatileDescriptorPool* _descriptorPool;
    std::optional<UniformMemoryPool::Session> _uniformMemorySession;

    using ImageStates = std::unordered_map<const Image*, ImageLayoutState>;
    ImageStates _imageStates;
  };

  inline CommandQueue& CommandProducer::queue() const noexcept
  {
    return _queue;
  }

  inline CommandBuffer* CommandProducer::commandBuffer() const noexcept
  {
    return _commandBuffer;
  }
}