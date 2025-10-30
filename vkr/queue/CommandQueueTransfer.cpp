#include <vkr/queue/CommandQueueTransfer.h>

using namespace mt;

CommandQueueTransfer::CommandQueueTransfer( Device& device,
                                            uint32_t familyIndex,
                                            uint32_t queueIndex,
                                            const QueueFamily& family,
                                            std::recursive_mutex& commonMutex) :
  CommandQueue(device, familyIndex, queueIndex, family, commonMutex)
{
}

std::unique_ptr<CommandProducerTransfer> CommandQueueTransfer::startCommands()
{
  std::lock_guard lock(commonMutex);
  return std::make_unique<CommandProducerTransfer>(commonPoolSet);
}
