#pragma once

#include <vkr/queue/CommandProducerTransfer.h>

namespace mt
{
  //  Продюсер команд, реализующий функционал компьют очереди
  //  Так же включает в себя функционал CommandProducerTransfer
  class CommandProducerCompute : public CommandProducerTransfer
  {
  public:
    //  Если debugName - не пустая строка, то все команды продюсера
    //  будут обернуты в vkCmdBeginDebugUtilsLabelEXT и
    //  vkCmdEndDebugUtilsLabelEXT
    CommandProducerCompute(CommandPoolSet& poolSet, const char* debugName);
    CommandProducerCompute(const CommandProducerCompute&) = delete;
    CommandProducerCompute& operator = (const CommandProducerCompute&) = delete;
    virtual ~CommandProducerCompute() noexcept = default;
  };
}
