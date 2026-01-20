#include <glm/gtc/matrix_transform.hpp>

#include <ddsSupport/ddsSupport.h>
#include <gui/ImGuiRAII.h>
#include <hld/FrameContext.h>
#include <hld/HLDLib.h>
#include <technique/TechniqueLoader.h>
#include <vkr/Device.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  GUIWindow(device,
            "Test window",
            std::nullopt,
            std::nullopt,
            VK_FORMAT_D16_UNORM),
  _frameTypeIndex(HLDLib::instance().getFrameTypeIndex(colorFrameType)),
  _cameraManipulator(CameraManipulator::APPLICATION_WINDOW_LOCATION),
  _meshAsset(new MeshAsset("Test mesh")),
  _drawable1(*_meshAsset),
  _drawable2(*_meshAsset),
  _drawable3(*_meshAsset),
  _drawStage(device),
  _commandMemoryPool(4 * 1024)
{
  _cameraManipulator.setCamera(&_camera);

  _setupMeshAsset();

  _scene.addDrawable(_drawable1);

  _scene.addDrawable(_drawable2);
  _drawable2.setPositionMatrix(glm::translate(glm::mat4(1), glm::vec3(3, 0, 0)));

  _scene.addDrawable(_drawable3);
  _drawable3.setPositionMatrix(glm::translate(glm::mat4(1), glm::vec3(0, 3, 0)));
}

void TestWindow::_setupMeshAsset()
{
  //  Техника отрисовки меша
  Ref configurator(new TechniqueConfigurator(device(), "Test technique"));
  loadConfigurator(*configurator, "examples/mesh/technique.tch");
  configurator->rebuildConfiguration();
  Ref technique(new Technique(*configurator));

  //  Создаем вершинный буфер
  glm::vec4 positions[36] = { {-1.0f, -1.0f, -1.0f, 1.0f},
                              {-1.0f,  1.0f, -1.0f, 1.0f},
                              { 1.0f,  1.0f, -1.0f, 1.0f},
                              {-1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f,  1.0f, -1.0f, 1.0f},
                              { 1.0f, -1.0f, -1.0f, 1.0f},

                              {-1.0f, -1.0f, 1.0f, 1.0f},
                              { 1.0f, -1.0f, 1.0f, 1.0f},
                              { 1.0f,  1.0f, 1.0f, 1.0f},
                              {-1.0f, -1.0f, 1.0f, 1.0f},
                              { 1.0f,  1.0f, 1.0f, 1.0f},
                              {-1.0f,  1.0f, 1.0f, 1.0f},

                              {-1.0f, -1.0f, -1.0f, 1.0f},
                              {-1.0f, -1.0f,  1.0f, 1.0f},
                              {-1.0f,  1.0f,  1.0f, 1.0f},
                              {-1.0f, -1.0f, -1.0f, 1.0f},
                              {-1.0f,  1.0f,  1.0f, 1.0f},
                              {-1.0f,  1.0f, -1.0f, 1.0f},

                              { 1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f,  1.0f, -1.0f, 1.0f},
                              { 1.0f,  1.0f,  1.0f, 1.0f},
                              { 1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f,  1.0f,  1.0f, 1.0f},
                              { 1.0f, -1.0f,  1.0f, 1.0f},

                              {-1.0f, 1.0f, -1.0f, 1.0f},
                              {-1.0f, 1.0f,  1.0f, 1.0f},
                              { 1.0f, 1.0f,  1.0f, 1.0f},
                              {-1.0f, 1.0f, -1.0f, 1.0f},
                              { 1.0f, 1.0f,  1.0f, 1.0f},
                              { 1.0f, 1.0f, -1.0f, 1.0f},

                              {-1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f, -1.0f,  1.0f, 1.0f},
                              {-1.0f, -1.0f, -1.0f, 1.0f},
                              { 1.0f, -1.0f,  1.0f, 1.0f},
                              {-1.0f, -1.0f,  1.0f, 1.0f}};

  Ref positionsBuffer(new DataBuffer( device(),
                                      sizeof(positions),
                                      DataBuffer::STORAGE_BUFFER,
                                      "Positions buffer"));
  device().graphicQueue()->uploadToBuffer(*positionsBuffer,
                                          0,
                                          sizeof(positions),
                                          positions);
  technique->getOrCreateResourceBinding("vertices").setBuffer(positionsBuffer);

  //  Создаем текстуру
  Ref<Image> image = loadDDS( "examples/image.dds",
                              device(),
                              nullptr,
                              true);
  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  technique->getOrCreateResourceBinding("colorTexture").setImage(imageView);

  //  Конфигурация ассета
  MeshAsset::Configuration meshConfig{};
  meshConfig.technique = technique;
  meshConfig.passes.push_back(MeshAsset::PassConfig{
                                        .frameTypeName = colorFrameType,
                                        .stageName = TestDrawStage::stageName,
                                        .layer = 0,
                                        .passName = "RenderPass"});
  meshConfig.vertexCount = sizeof(positions) / sizeof(positions[0]);
  meshConfig.maxInstancesCount = 2;

  _meshAsset->setConfiguration(meshConfig);
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
  frameContext.viewCamera = &_camera;

  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  _drawStage.draw(frameContext);
  drawGUI(commandProducer);

  renderPass.endPass();
}

void TestWindow::guiImplementation()
{
  GUIWindow::guiImplementation();

  _cameraManipulator.update(ImVec2(0.0f, 0.0f),
                            ImVec2((float)size().x, (float)size().y));
}
