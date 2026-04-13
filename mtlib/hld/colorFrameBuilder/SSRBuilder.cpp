#include <hld/colorFrameBuilder/SSRBuilder.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

SSRBuilder::SSRBuilder(Device& device) :
  _device(device),
  _technique(device, "ssr/buildSSR.tch"),
  _marchingPass(_technique.getOrCreatePass("MarchingPass")),
  _reflectionBufferBinding(
    _technique.getOrCreateResourceBinding("outReflectionBuffer")),
  _prevHDRBufferBinding(_technique.getOrCreateResourceBinding("prevHDR")),
  _gridSize(1)
{
}

void SSRBuilder::buildReflection(CommandProducerGraphic& commandProducer)
{
  MT_ASSERT(_reflectionBufferBinding.image() != nullptr);

  {
    Technique::BindCompute bind(_technique, _marchingPass, commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(_gridSize);
  }
}
