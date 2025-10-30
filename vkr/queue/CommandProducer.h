#pragma once

#include <optional>
#include <span>
#include <utility>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vkr/queue/ImageAccessWatcher.h>
#include <vkr/queue/UniformMemoryPool.h>

namespace mt
{
  class CommandBuffer;
  class CommandPool;
  class CommandPoolSet;
  class CommandQueue;
  class ImageSlice;
  class PlainBuffer;
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

    //  Копирование из буфера в Image
    //  srcBuffer должен быть создан с Usage = UPLOAD_BUFFER или
    //    bufferUsageFlags VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    //  dstImage должен быть создан с usageFlags VK_IMAGE_USAGE_TRANSFER_DST_BIT
    //  srcRowLength - ширина изображения, записанного в буфере, в текселях.
    //    Используется для определения начала следующей строки при копировании
    //    2D и 3D изображений, когда dstExtent не совпадает с размерами
    //    изображения в буфере.
    //  srcImageHeight - высота изображения, записанного в буфере, в текселях.
    //    Используется для определения начала следующего слоя при копировании 3D
    //    изображений, когда dstExtent не совпадает с размерами изображения
    //    в буфере.
    void copyFromBufferToImage( const PlainBuffer& srcBuffer,
                                VkDeviceSize srcBufferOffset,
                                uint32_t srcRowLength,
                                uint32_t srcImageHeight,
                                const Image& dstImage,
                                VkImageAspectFlags dstAspectMask,
                                uint32_t dstArrayIndex,
                                uint32_t dstMipLevel,
                                glm::uvec3 dstOffset,
                                glm::uvec3 dstExtent);

    //  Копирование из Image в буфер
    //  srcImage должен быть создан с usageFlags VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    //  dstBuffer должен быть создан с Usage = DOWNLOAD_BUFFER или
    //    bufferUsageFlags VK_BUFFER_USAGE_TRANSFER_DST_BIT
    //  dstRowLength - ширина изображения, которое уже есть в буфере, в
    //    текселях. Используется для определения начала следующей строки при
    //    копировании 2D и 3D изображений, когда dstExtent не совпадает с
    //    размерами изображения в буфере.
    //  dstImageHeight - высота изображения, которое уже есть в буфере, в
    //    текселях. Используется для определения начала следующего слоя при
    //    копировании 3D изображений, когда dstExtent не совпадает с размерами
    //    изображения в буфере.
    void copyFromImageToBuffer( const Image& srcImage,
                                VkImageAspectFlags srcAspectMask,
                                uint32_t srcArrayIndex,
                                uint32_t srcMipLevel,
                                glm::uvec3 srcOffset,
                                glm::uvec3 srcExtent,
                                const PlainBuffer& dstBuffer,
                                VkDeviceSize dstBufferOffset,
                                uint32_t dstRowLength,
                                uint32_t dstImageHeight);

    //  Обертка вокруг vkCmdBlitImage
    //  Копировать один кусок Image в другое место (возможно в этот же Image,
    //    возможно в другой). Возможно преобразование формата и разрешения.
    //    последним
    //  Ограничения см. в описании vkCmdBlitImage
    void blitImage( const Image& srcImage,
                    VkImageAspectFlags srcAspect,
                    uint32_t srcArrayIndex,
                    uint32_t srcMipLevel,
                    const glm::uvec3& srcOffset,
                    const glm::uvec3& srcExtent,
                    const Image& dstImage,
                    VkImageAspectFlags dstAspect,
                    uint32_t dstArrayIndex,
                    uint32_t dstMipLevel,
                    const glm::uvec3& dstOffset,
                    const glm::uvec3& dstExtent,
                    VkFilter filter);

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
      CommandBuffer* matchingBuffer = nullptr;

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

    //  Половина трансфера владения (vulkan Queue Family Ownership Transfer)
    //  Команды, исполняемые на каждой из очередей, участвующих в трансфере
    void halfOwnershipTransfer( const Image& image,
                                uint32_t oldFamilyIndex,
                                uint32_t newFamilyIndex);

    //  Половина трансфера владения (vulkan Queue Family Ownership Transfer)
    //  Команды, исполняемые на каждой из очередей, участвующих в трансфере
    void halfOwnershipTransfer( const PlainBuffer& buffer,
                                uint32_t oldFamilyIndex,
                                uint32_t newFamilyIndex);

  private:
    CommandBuffer& _getOrCreateBuffer();
    void _addImageUsage(const Image& image, const ImageAccess& access);

    using ImageUsage = std::pair<const Image*, const ImageAccess*>;
    using MultipleImageUsage = std::span<ImageUsage>;
    void _addMultipleImagesUsage(MultipleImageUsage usages);

    void _addBufferUsage(const PlainBuffer& buffer);

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
    CommandBuffer* _cachedMatchingBuffer;

    bool _isFinalized;
  };

  inline CommandQueue& CommandProducer::queue() const noexcept
  {
    return _queue;
  }
}