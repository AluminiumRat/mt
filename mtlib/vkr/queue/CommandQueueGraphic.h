#pragma once

#include <vkr/queue/CommandQueueCompute.h>
#include <vkr/queue/CommandProducerGraphic.h>

namespace mt
{
  //  Графическая очередь, то есть очередь, отвечающая за отрисовку. Как правило
  //  основная очередь, через которую происходит общение с GPU.
  class CommandQueueGraphic : public CommandQueueCompute
  {
  protected:
    // Логический девайс создает очереди в конструкторе. Это единственный
    // способ создания очередей.
    friend class Device;
    CommandQueueGraphic(Device& device,
                        uint32_t familyIndex,
                        uint32_t queueIndex,
                        const QueueFamily& family,
                        std::recursive_mutex& commonMutex);
  public:
    CommandQueueGraphic(const CommandQueue&) = delete;
    CommandQueueGraphic& operator = (const CommandQueue&) = delete;
    virtual ~CommandQueueGraphic() noexcept = default;

    //  Начать заполнение буфера команд.
    //  Более детально смотри CommandQueue::startCommands
    std::unique_ptr<CommandProducerGraphic> startCommands();

    //  Отправить собранный в продюсере буфер команд на исполнение и
    //    финализировать работу с продюсером.
    inline void submitCommands(
                            std::unique_ptr<CommandProducerGraphic> producer);
  };

  inline void CommandQueueGraphic::submitCommands(
                              std::unique_ptr<CommandProducerGraphic> producer)
  {
    CommandQueue::submitCommands(
                          std::unique_ptr<CommandProducer>(producer.release()));
  }
};