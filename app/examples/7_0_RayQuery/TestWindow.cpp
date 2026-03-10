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
  loadConfigurator(*_configurator, "examples/rayTracing/rayQueryTest.tch");
  _configurator->rebuildConfiguration();

  BLASImporter importer(*device.graphicQueue());
  _blasInstances = importer.import("examples/Duck/glTF/Duck.gltf");
  MT_ASSERT(!_blasInstances.empty());
  _tlas = new TLAS(device, _blasInstances, "testTLAS");

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

  // Анимация в TLAS
  static int frameIndex = 0;
  _blasInstances[0].transform[3] = glm::vec4(sin(frameIndex++ / 20.f), 0, 0, 1);
  _blasInstances[0].mask = frameIndex % 200 > 180 ? 0 : 0xFF;
  _tlas->update(_blasInstances, *commandProducer);

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}
