#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

#include <BLASImporter.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow( device,
                "Test window",
                std::nullopt,
                std::nullopt,
                VK_FORMAT_UNDEFINED),
  _configurator(new TechniqueConfigurator(device, "Test technique")),
  _technique(*_configurator),
  _tlasBinding(_technique.getOrCreateResourceBinding("tlas")),
  _pass(_technique.getOrCreatePass("RenderPass"))
{
  loadConfigurator(*_configurator, "examples/rayTracing/intersectionTest.tch");
  _configurator->rebuildConfiguration();

  BLASImporter importer(*device.graphicQueue());
  BLASInstances importedInstances =
                                importer.import("examples/Duck/glTF/Duck.gltf");
  MT_ASSERT(!importedInstances.empty());
  _tlas = new TLAS(device, importedInstances, "testTLAS");

  std::unique_ptr<CommandProducerGraphic> producer =
                                        device.graphicQueue()->startCommands();
  _tlas->build(*producer);
  device.graphicQueue()->submitCommands(std::move(producer));

  _tlasBinding.setTLAS(_tlas);
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                      device().graphicQueue()->startCommands();

  CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);
    Technique::BindGraphic bind(_technique, _pass, *commandProducer);
    if(bind.isValid())
    {
      commandProducer->draw(4);
      bind.release();
    }
  renderPass.endPass();

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}
