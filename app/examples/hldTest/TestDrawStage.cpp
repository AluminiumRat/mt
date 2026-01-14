#include <hld/drawCommand/DrawCommandList.h>
#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>
#include <vkr/queue/CommandProducerGraphic.h>

#include <TestDrawable.h>
#include <TestDrawStage.h>

using namespace mt;

TestDrawStage::TestDrawStage() :
  DrawStage(stageName)
{
}

void TestDrawStage::drawImplementation(FrameContext& frameContext) const
{
  CommandMemoryPool commandsPool(1024);
  DrawCommandList commands(commandsPool);

  const std::vector<const Drawable*>& drawables =
                                frameContext.drawPlan->stagePlan(stageIndex());
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
}
