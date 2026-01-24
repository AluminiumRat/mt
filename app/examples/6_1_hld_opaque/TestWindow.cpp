#include <glm/gtc/matrix_transform.hpp>

#include <ddsSupport/ddsSupport.h>
#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <gui/ImGuiRAII.h>
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
  _frameBuilder(device),
  _cameraManipulator(CameraManipulator::APPLICATION_WINDOW_LOCATION),
  _meshAsset(new MeshAsset("Test mesh")),
  _illumination(device)
{
  _cameraManipulator.setCamera(&_camera);

  _setupMeshAsset();
  _fillScene();
}

void TestWindow::_setupMeshAsset()
{
  //  Техника отрисовки меша
  Ref configurator(new TechniqueConfigurator(device(), "Test technique"));
  loadConfigurator(*configurator, "lib/opaqueMesh/opaqueMesh.tch");
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
                              .frameTypeName = ColorFrameBuilder::frameTypeName,
                              .stageName = OpaqueColorStage::stageName,
                              .layer = 0,
                              .passName = "RenderPass"});
  meshConfig.vertexCount = sizeof(positions) / sizeof(positions[0]);
  meshConfig.maxInstancesCount = 2;

  meshConfig.boundingBox = AABB(-1, -1, -1, 1, 1, 1);

  _meshAsset->setConfiguration(meshConfig);
}

void TestWindow::_fillScene()
{
  for(int i = -2; i <=2; i++)
  {
    for(int j = -2; j <= 2; j++)
    {
      std::unique_ptr<MeshDrawable> newDrawable(new MeshDrawable(*_meshAsset));
      _scene.registerDrawable(*newDrawable);
      glm::mat4 translete =
                      glm::translate(glm::mat4(1), glm::vec3(i * 3, j * 3, 0));
      newDrawable->setPositionMatrix(translete);
      _drawables.push_back(std::move(newDrawable));
    }
  }
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  _illumination.update();
  _frameBuilder.draw( frameBuffer,
                      _scene,
                      _camera,
                      _illumination,
                      [this](CommandProducerGraphic& commandProducer)
                      {
                        drawGUI(commandProducer);
                      },
                      commandProducer);
}

void TestWindow::guiImplementation()
{
  GUIWindow::guiImplementation();

  _cameraManipulator.update(ImVec2(0.0f, 0.0f),
                            ImVec2((float)size().x, (float)size().y));

  ImGuiWindow statisticWindow("Statistic");
  if(statisticWindow.visible())
  {
    ImGui::Text("Hello!");
    statisticWindow.end();
  }
}
