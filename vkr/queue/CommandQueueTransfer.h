#pragma once

#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/CommandProducerTransfer.h>

namespace mt
{
  //  Трансфер очередь, то есть очередь, специально предназначенная для
  //  асинхронных перемещений данных по памяти
  class CommandQueueTransfer : public CommandQueue
  {
  private:
    // Логический девайс создает очереди в конструкторе. Это единственный
    // способ создания очередей.
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