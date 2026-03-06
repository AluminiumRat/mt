#include <vkr/Device.h>

#include <BLASImporter.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow( device,
                "Test window",
                std::nullopt,
                std::nullopt,
                VK_FORMAT_UNDEFINED)
{
  BLASImporter importer(*device.graphicQueue());
  importer.import("examples/Duck/glTF/Duck.gltf");
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                      device().graphicQueue()->startCommands();

  CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);
  renderPass.endPass();

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}
