#pragma once

#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/CommandProducerTransfer.h>

namespace mt
{
  //  Трансфер очередь, то есть очередь, специально предназначенная для
  //  асинхронных перемещений данных по памяти
  class CommandQueueTransfer : public CommandQueue
  {
  protected:
    //  Логический девайс создает очереди в своем конструкторе. Это единственный
    //  способ создания очередей.
    friend class Device;
    CommandQueueTransfer( Device& device,
                          uint32_t familyIndex,
                          uint32_t queueIndex,
                          const QueueFamily& family,
                          std::recursive_mutex& commonMutex);
  public:
    CommandQueueTransfer(const CommandQueue&) = delete;
    CommandQueueTransfer& operator = (const CommandQueue&) = delete;
    virtual  ~CommandQueueTransfer() noexcept = default;

    //  Загрузить дынные в буфер.
    //  Операция асинхронная, но вы можете подождать её окончания с помощью
    //    синк поинта(метод createSyncPoint)
    //  Если dstBuffer создается с явным указанием VkBufferUsageFlags, то для
    //    него должен быть включен VK_BUFFER_USAGE_TRANSFER_DST_BIT
    void uploadToBuffer(const DataBuffer& dstBuffer,
                        size_t shiftInDstBuffer,
                        size_t dataSize,
                        void* srcData);

    //  Начать заполнение буфера команд.
    //  Более детально смотри CommandQueue::startCommands
    std::unique_ptr<CommandProducerTransfer> startCommands();

    //  Отправить собранный в продюсере буфер команд на исполнение и
    //    финализировать работу с продюсером.
    inline void submitCommands(
                            std::unique_ptr<CommandProducerTransfer> producer);
  };

  inline void CommandQueueTransfer::submitCommands(
                              std::unique_ptr<CommandProducerTransfer> producer)
  {
    CommandQueue::submitCommands(
                          std::unique_ptr<CommandProducer>(producer.release()));
  }
};