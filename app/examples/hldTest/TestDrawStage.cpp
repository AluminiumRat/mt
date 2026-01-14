#include <hld/drawCommand/DrawCommandList.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>
#include <hld/HLDLib.h>
#include <vkr/queue/CommandProducerGraphic.h>

#include <TestDrawable.h>
#include <TestDrawStage.h>

using namespace mt;

TestDrawStage::TestDrawStage() :
  _stageIndex(HLDLib::instance().getStageIndex(stageName))
{
}

void TestDrawStage::draw(FrameContext& frameContext) const
{
  frameContext.commandProducer->beginDebugLabel(stageName);

  frameContext.drawStageIndex = _stageIndex;

  DrawCommandList commands(*frameContext.commandMemoryPool);

  const std::vector<const Drawable*>& drawables =
                                  frameContext.drawPlan->stagePlan(_stageIndex);
  for(const Drawable* drawable : drawables)
  {
    const TestDrawable* testDrawable =
                                      static_cast<const TestDrawable*>(drawable);
    testDrawable->addToCommandList(commands, frameContext);
  }

  CommandProducerGraphic::RenderPass renderPass(*frameContext.commandProducer,
                                                *frameContext.frameBuffer);

  commands.draw(*frameContext.commandProducer,
                DrawCommandList::FAR_FIRST_SORTING);

  renderPass.endPass();

  frameContext.commandProducer->endDebugLabel();
}
