#include <hld/DrawStage.h>
#include <hld/FrameContext.h>
#include <hld/HLDLib.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

DrawStage::DrawStage(const char* name) :
  _name(name),
  _stageIndex(HLDLib::instance().getStageIndex(name))
{
}

void DrawStage::draw(FrameContext& frameContext) const
{
  const DrawStage* oldStage = frameContext.drawStage;
  frameContext.drawStage = this;

  try
  {
    frameContext.commandProducer->beginDebugLabel(_name.c_str());
    setCommonSet(frameContext);
    drawImplementation(frameContext);
    frameContext.commandProducer->endDebugLabel();
  }
  catch(...)
  {
    frameContext.drawStage = oldStage;
    throw;
  }
}

void DrawStage::setCommonSet(FrameContext& frameContext) const
{
}

void DrawStage::drawImplementation(FrameContext& frameContext) const
{
}
