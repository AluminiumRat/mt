#include <vkr/queue/CommandProducerCompute.h>

using namespace mt;

CommandProducerCompute::CommandProducerCompute( CommandPoolSet& poolSet,
                                                const char* debugName) :
  CommandProducerTransfer(poolSet, debugName)
{
}
