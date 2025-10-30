#include <vkr/queue/CommandQueueCompute.h>

using namespace mt;

CommandQueueCompute::CommandQueueCompute( Device& device,
                                          uint32_t familyIndex,
                                          uint32_t queueIndex,
                                          const QueueFamily& family,
                                          std::recursive_mutex& commonMutex) :
  CommandQueue(device, familyIndex, queueIndex, family, commonMutex)
{
}

std::unique_ptr<CommandProducerCompute> CommandQueueCompute::startCommands()
{
  std::lock_guard lock(commonMutex);
  return std::make_unique<CommandProducerCompute>(commonPoolSet);
}
