#include <vkr/queue/CommandQueueGraphic.h>

using namespace mt;

CommandQueueGraphic::CommandQueueGraphic( Device& device,
                                          uint32_t familyIndex,
                                          uint32_t queueIndex,
                                          const QueueFamily& family,
                                          std::recursive_mutex& commonMutex) :
  CommandQueueCompute(device, familyIndex, queueIndex, family, commonMutex)
{
}

std::unique_ptr<CommandProducerGraphic> CommandQueueGraphic::startCommands(
                                                  const char* producerDebugName)
{
  std::lock_guard lock(commonMutex);
  return std::make_unique<CommandProducerGraphic>(commonPoolSet,
                                                  producerDebugName);
}
