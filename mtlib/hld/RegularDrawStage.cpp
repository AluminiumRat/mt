#include <hld/drawScene/Drawable.h>
#include <hld/DrawPlan.h>
#include <hld/HLDLib.h>
#include <hld/RegularDrawStage.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

RegularDrawStage::RegularDrawStage( Device& device,
                                    const char* stageName,
                                    DrawCommandList::Sorting sorting) :
  _device(device),
  _stageIndex(HLDLib::instance().getStageIndex(stageName)),
  _commandMemoryPool(4 * 1024),
  _drawCommands(_commandMemoryPool),
  _sorting(sorting)
{
}

void RegularDrawStage::draw(CommandProducerGraphic& commandProducer,
                            const DrawPlan& drawPlan,
                            const FrameBuildContext& frameContext,
                            glm::uvec2 viewport)
{
  MT_ASSERT(viewport.x != 0 && viewport.y != 0);

  _drawCommands.clear();
  _commandMemoryPool.reset();

  _drawCommands.fillFromStagePlan(drawPlan.stagePlan(_stageIndex),
                                  frameContext,
                                  _stageIndex,
                                  nullptr);

  if(_frameBuffer == nullptr) _frameBuffer = buildFrameBuffer();
  MT_ASSERT(_frameBuffer != nullptr);

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                *_frameBuffer,
                                                std::nullopt,
                                                viewport);
  _drawCommands.draw(commandProducer, _sorting);
  renderPass.endPass();
}