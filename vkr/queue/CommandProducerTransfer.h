#pragma once

#include <vkr/queue/CommandProducer.h>

namespace mt
{
  //  Продюсер команд, реализующий функционал трансфер очереди
  class CommandProducerTransfer : public CommandProducer
  {
  public:
    CommandProducerTransfer(CommandPoolSet& poolSet);
    CommandProducerTransfer(const CommandProducerTransfer&) = delete;
    CommandProducerTransfer& operator = (
                                      const CommandProducerTransfer&) = delete;
    virtual ~CommandProducerTransfer() noexcept = default;

    //  Копирование из одного буфера в другой
    //  srcBuffer должен быть создан с Usage = UPLOADING_BUFFER или
    //    Usage = VOLATILE_UNIFORM_BUFFER или
    //    bufferUsageFlags VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    //  dstBuffer должен быть создан с Usage отличным от UPLOADING_BUFFER
    //    и VOLATILE_UNIFORM_BUFFER, или с bufferUsageFlags с выставленным
    //    VK_BUFFER_USAGE_TRANSFER_SRC_BIT.
    void copyFromBufferToBuffer(const DataBuffer& srcBuffer,
                                const DataBuffer& dstBuffer,
                                size_t srcOffset,
                                size_t dstOffset,
                                size_t size);

    //  Специальный метод для копирования данных в статический юниформ буфер
    //  Операция копирования записывается в буфер подготовительных команд. То
    //    есть она будет выполнена до основной последовательности команд и будет
    //    происходить вне рендер пассов (где команды копирования запрещены)
    //  srcBuffer должен быть создан с Usage = UPLOADING_BUFFER или
    //    Usage = VOLATILE_UNIFORM_BUFFER или
    //    bufferUsageFlags VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    //  dstBuffer должен быть создан с Usage отличным от UPLOADING_BUFFER
    //    и VOLATILE_UNIFORM_BUFFER, или с bufferUsageFlags с выставленным
    //    VK_BUFFER_USAGE_TRANSFER_SRC_BIT.
    void uniformBufferTransfer( const DataBuffer& srcBuffer,
                                const DataBuffer& dstBuffer,
                                size_t srcOffset,
                                size_t dstOffset,
                                size_t size);

    //  Копирование из буфера в Image
    //  srcBuffer должен быть создан с Usage = UPLOADING_BUFFER или
    //    bufferUsageFlags VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    //  dstImage должен быть создан с usageFlags VK_IMAGE_USAGE_TRANSFER_DST_BIT
    //  srcRowLength - ширина изображения, записанного в буфере, в текселях.
    //    Используется для определения начала следующей строки при копировании
    //    2D и 3D изображений, когда dstExtent не совпадает с размерами
    //    изображения в буфере. Для 1D изображений можно передать 0.
    //  srcImageHeight - высота изображения, записанного в буфере, в текселях.
    //    Используется для определения начала следующего слоя при копировании 3D
    //    изображений, когда dstExtent не совпадает с размерами изображения
    //    в буфере. Для 1D и 2D изображений можно передать 0
    void copyFromBufferToImage( const DataBuffer& srcBuffer,
                                VkDeviceSize srcBufferOffset,
                                uint32_t srcRowLength,
                                uint32_t srcImageHeight,
                                const Image& dstImage,
                                VkImageAspectFlags dstAspectMask,
                                uint32_t dstArrayIndex,
                                uint32_t dstArrayCount,
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
    //    размерами изображения в буфере. Для 1D изображений можно передать 0.
    //  dstImageHeight - высота изображения, которое уже есть в буфере, в
    //    текселях. Используется для определения начала следующего слоя при
    //    копировании 3D изображений, когда dstExtent не совпадает с размерами
    //    изображения в буфере. Для 1D и 2D изображений можно передать 0
    void copyFromImageToBuffer( const Image& srcImage,
                                VkImageAspectFlags srcAspectMask,
                                uint32_t srcArrayIndex,
                                uint32_t srcMipLevel,
                                glm::uvec3 srcOffset,
                                glm::uvec3 srcExtent,
                                const DataBuffer& dstBuffer,
                                VkDeviceSize dstBufferOffset,
                                uint32_t dstRowLength,
                                uint32_t dstImageHeight);
  };
};