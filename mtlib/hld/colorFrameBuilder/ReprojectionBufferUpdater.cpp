#include <hld/colorFrameBuilder/ReprojectionBufferUpdater.h>
#include <technique/TechniqueLoader.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

ReprojectionBufferUpdater::ReprojectionBufferUpdater(Device& device) :
  _techniqueConfigurator(new TechniqueConfigurator(
                                                  device,
                                                  "ReprojectionBufferUpdate")),
  _technique(*_techniqueConfigurator),
  _pass(_technique.getOrCreatePass("UpdatePass")),
  _reprojectionBufferBinding(
                _technique.getOrCreateResourceBinding("outReprojectionBuffer")),
  _gridSize(1)
{
  loadConfigurator( *_techniqueConfigurator,
                    "reprojectionBuffer/updateReprojectionBuffer.tch");
  _techniqueConfigurator->rebuildConfiguration();
}

void ReprojectionBufferUpdater::updateReprojection(
                                        CommandProducerGraphic& commandProducer)
{
  Technique::BindCompute bind(_technique, _pass, commandProducer);
  MT_ASSERT(bind.isValid())
  commandProducer.dispatch(_gridSize);
}
