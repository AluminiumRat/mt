#include <hld/colorFrameBuilder/HiZBuilder.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/DataBuffer.h>
#include <vkr/Device.h>

using namespace mt;

HiZBuilder::HiZBuilder(Device& device) :
  _technique(device, "hiZ/buildHiZ.tch"),
  _buildPass(_technique.getOrCreatePass("BuildPass")),
  _hiZBinding(_technique.getOrCreateResourceBinding("hiZ")),
  _hizSizeUniform(_technique.getOrCreateUniform("params.hiZSize")),
  _gridSize(1)
{
}

void HiZBuilder::buildHiZ(CommandProducerCompute& producer)
{
  MT_ASSERT(_hiZ != nullptr);

  Technique::BindCompute bind(_technique, _buildPass, producer);
  MT_ASSERT(bind.isValid());
  producer.dispatch(_gridSize);
}
