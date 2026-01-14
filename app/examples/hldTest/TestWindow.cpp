#include <hld/FrameContext.h>
#include <hld/HLDLib.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Test window"),
  _frameTypeIndex(HLDLib::instance().getFrameTypeIndex(colorFrameType)),
  _drawable(device, 1),
  _drawable2(device, 2),
  _commandMemoryPool(4 * 1024)
{
  _camera.setPerspectiveProjection(1, 1, 0.1f, 100);
  _scene.addDrawable(_drawable);
  _scene.addDrawable(_drawable2);
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  _drawPlan.clear();
  _commandMemoryPool.reset();

  _scene.fillDrawPlan(_drawPlan, _camera, _frameTypeIndex);

  FrameContext frameContext{};
  frameContext.frameTypeIndex = _frameTypeIndex;
  frameContext.drawPlan = &_drawPlan;
  frameContext.commandMemoryPool = &_commandMemoryPool;
  frameContext.frameBuffer = &frameBuffer;
  frameContext.commandProducer = &commandProducer;

  _drawStage.draw(frameContext);
}
