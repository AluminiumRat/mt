#include <hld/colorFrameBuilder/VelocityBufferUpdater.h>
#include <technique/TechniqueLoader.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

VelocityBufferUpdater::VelocityBufferUpdater(Device& device) :
  _techniqueConfigurator(new TechniqueConfigurator( device,
                                                    "VelocityBufferUpdate")),
  _technique(*_techniqueConfigurator),
  _pass(_technique.getOrCreatePass("UpdatePass")),
  _velocityBufferBinding(
                    _technique.getOrCreateResourceBinding("outVelocityBuffer"))
{
  loadConfigurator( *_techniqueConfigurator,
                    "velocityBuffer/updateVelocityBuffer.tch");
  _techniqueConfigurator->rebuildConfiguration();
}

void VelocityBufferUpdater::updateVelocityBuffer(
                                        CommandProducerGraphic& commandProducer,
                                        const ImageView& velocityBuffer)
{
  _velocityBufferBinding.setImage(&velocityBuffer);

  glm::uvec2 gridSize =
                    (glm::uvec2(velocityBuffer.extent()) + glm::uvec2(7)) / 8u;
  Technique::BindCompute bind(_technique, _pass, commandProducer);
  MT_ASSERT(bind.isValid())
  commandProducer.dispatch(gridSize);
}
