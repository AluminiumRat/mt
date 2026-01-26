#pragma once

#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/queue/CommandProducerCompute.h>

namespace mt
{
  //  Компьют очередь, то есть очередь, специально отведенная под асинхронное
  //  выполнение компьют шейдеров
  class CommandQueueCompute : public CommandQueueTransfer
  {
  protected:
    // Логический девайс создает очереди в конструкторе. Это единственный
    // способ создания очередей.
    friend class Device;
    CommandQueueCompute(Device& device,
                        uint32_t familyIndex,
                        uint32_t queueIndex,
                        const QueueFamily& family,
                        std::recursive_mutex& commonMutex);
  public:
    CommandQueueCompute(const CommandQueue&) = delete;
    CommandQueueCompute& operator = (const CommandQueue&) = delete;
    virtual  ~CommandQueueCompute() noexcept = default;

    //  Начать заполнение буфера команд.
    //  Более детально смотри CommandQueue::startCommands
    std::unique_ptr<CommandProducerCompute> startCommands(
                                            const char* producerDebugName = "");

    //  Отправить собранный в продюсере буфер команд на исполнение и
    //    финализировать работу с продюсером.
    inline void submitCommands(
                              std::unique_ptr<CommandProducerCompute> producer);
  };

  inline void CommandQueueCompute::submitCommands(
                              std::unique_ptr<CommandProducerCompute> producer)
  {
    CommandQueue::submitCommands(
                          std::unique_ptr<CommandProducer>(producer.release()));
  }
};