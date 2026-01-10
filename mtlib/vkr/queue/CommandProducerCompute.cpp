#include <vkr/queue/CommandProducerCompute.h>

using namespace mt;

CommandProducerCompute::CommandProducerCompute(CommandPoolSet& poolSet) :
  CommandProducerTransfer(poolSet)
{
}
