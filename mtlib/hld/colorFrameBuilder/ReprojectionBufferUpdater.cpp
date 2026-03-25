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
                _technique.getOrCreateResourceBinding("outReprojectionBuffer"))
{
  loadConfigurator( *_techniqueConfigurator,
                    "reprojectionBuffer/updateReprojectionBuffer.tch");
  _techniqueConfigurator->rebuildConfiguration();
}

void ReprojectionBufferUpdater::updateReprojection(
                                        CommandProducerGraphic& commandProducer,
                                        const ImageView& reprojectionBuffer)
{
  _reprojectionBufferBinding.setImage(&reprojectionBuffer);

  glm::uvec2 gridSize =
                (glm::uvec2(reprojectionBuffer.extent()) + glm::uvec2(7)) / 8u;
  Technique::BindCompute bind(_technique, _pass, commandProducer);
  MT_ASSERT(bind.isValid())
  commandProducer.dispatch(gridSize);
}
