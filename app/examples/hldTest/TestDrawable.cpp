#include <ddsSupport/ddsSupport.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/drawCommand/DrawCommand.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>
#include <hld/HLDLib.h>
#include <technique/TechniqueLoader.h>
#include <vkr/Device.h>

#include <TestDrawable.h>
#include <TestDrawStage.h>
#include <TestWindow.h>

using namespace mt;

class TestDrawCommand : public DrawCommand
{
public:
  TestDrawCommand(const TestDrawable& drawable,
                  uint32_t groupIndex,
                  float distance) :
    DrawCommand(groupIndex, 0, distance),
    _drawable(drawable)
  {
  }

  virtual void draw(CommandProducerGraphic& producer,
                    std::span<const CommandPtr> commands) override
  {
    for(const CommandPtr& command : commands)
    {
      const TestDrawCommand& testCommand =
                                  static_cast<const TestDrawCommand&>(*command);
      testCommand._drawable.draw(producer);
    }
  }

private:
  const TestDrawable& _drawable;
};

TestDrawable::TestDrawable(Device& device, float distance) :
  _distance(distance),
  _colorFrameType(HLDLib::instance().getFrameTypeIndex(TestWindow::colorFrameType)),
  _drawStage(HLDLib::instance().getStageIndex(TestDrawStage::stageName)),
  _drawCommandGroup(HLDLib::instance().allocateDrawCommandGroupIndex()),
  _configurator(new TechniqueConfigurator(device, "Test technique")),
  _technique(new Technique(*_configurator)),
  _pass(_technique->getOrCreatePass("RenderPass")),
  _vertexBuffer(_technique->getOrCreateResourceBinding("vertices")),
  _texture(_technique->getOrCreateResourceBinding("colorTexture"))
{
  loadConfigurator(*_configurator, "examples/dds_load/technique.tch");
  _configurator->rebuildConfiguration();

  _createVertexBuffer(device);
  _createTexture(device);
}

void TestDrawable::_createVertexBuffer(Device& device)
{
  struct Vertex
  {
    alignas(16) glm::vec3 position;
  };
  Vertex vertices[4] =  { {.position = {-0.5f, -0.5f, 0.0f}},
                          {.position = {-0.5f, 0.5f, 0.0f}},
                          {.position = {0.5f, -0.5f, 0.0f}},
                          {.position = {0.5f, 0.5f, 0.0f}}};
  Ref<DataBuffer> buffer(new DataBuffer(device,
                                        sizeof(vertices),
                                        DataBuffer::STORAGE_BUFFER,
                                        "Vertex buffer"));
  device.graphicQueue()->uploadToBuffer(*buffer,
                                        0,
                                        sizeof(vertices),
                                        vertices);
  _vertexBuffer.setBuffer(buffer);
}

void TestDrawable::_createTexture(Device& device)
{
  Ref<Image> image = loadDDS( "examples/image.dds",
                              device,
                              nullptr,
                              true);

  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  _texture.setImage(imageView);
}

void TestDrawable::addToDrawPlan( DrawPlan& plan,
                                  uint32_t frameTypeIndex) const
{
  if(frameTypeIndex == _colorFrameType) plan.addDrawable(*this, _drawStage);
}

void TestDrawable::addToCommandList(DrawCommandList& commandList,
                                    const FrameContext& frameContext) const
{
  if (frameContext.frameTypeIndex != _colorFrameType) return;
  if (frameContext.drawStageIndex != _drawStage) return;

  commandList.createCommand<TestDrawCommand>( *this,
                                              _drawCommandGroup,
                                              _distance);
  float distance2 = _distance + 10;
  commandList.createCommand<TestDrawCommand>( *this,
                                              _drawCommandGroup,
                                              distance2);
}

void TestDrawable::draw(CommandProducerGraphic& commandProducer) const
{
  Technique::Bind bind(*_technique, _pass, commandProducer);
  if (bind.isValid())
  {
    commandProducer.draw(4);
    bind.release();
  }
}