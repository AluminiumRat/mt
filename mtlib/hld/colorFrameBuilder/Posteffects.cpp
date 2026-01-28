#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/Posteffects.h>
#include <hld/FrameBuildContext.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

Posteffects::Posteffects(Device& device, TechniqueManager& techniqueManager) :
  _device(device),
  _techniqueManager(techniqueManager),
  _brightnessPyramid(_device)
{
}

void Posteffects::prepare(CommandProducerGraphic& commandProducer,
                          const FrameBuildContext& frameContext)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  _brightnessPyramid.update(commandProducer, *_hdrBuffer);
}

void Posteffects::makeLDR(CommandProducerGraphic& commandProducer,
                          const FrameBuildContext& frameContext)
{
}
