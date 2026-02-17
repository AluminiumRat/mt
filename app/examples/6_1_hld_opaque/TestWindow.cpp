#include <glm/gtc/matrix_transform.hpp>

#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <imageIO/imageIO.h>
#include <technique/TechniqueLoader.h>
#include <vkr/Device.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  GUIWindow(device,
            "Test window",
            std::nullopt,
            std::nullopt,
            VK_FORMAT_UNDEFINED),
  _frameBuilder(device),
  _cameraManipulator(CameraManipulator::APPLICATION_WINDOW_LOCATION),
  _meshAsset(new MeshAsset("Test mesh")),
  _environment(device)
{
  setNeedClearFrameBuffer(false);

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
  std::unique_ptr<Technique> technique(new Technique(*configurator));

  //  Создаем текстуру
  Ref<Image> image = loadImage( "examples/image.dds",
                                *device().graphicQueue(),
                                false);
  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  technique->getOrCreateResourceBinding("colorTexture").setImage(imageView);

  _meshAsset->addTechnique(std::move(technique));

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

  _meshAsset->setCommonBuffer("vertices", *positionsBuffer);
  _meshAsset->setVertexCount(sizeof(positions) / sizeof(positions[0]));
  _meshAsset->setBound(AABB(-1, -1, -1, 1, 1, 1));
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

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  _frameBuilder.draw( frameBuffer,
                      _scene,
                      _camera,
                      _environment);

  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                device().graphicQueue()->startCommands("ImGui");
    CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);
    drawGUI(*commandProducer);
    renderPass.endPass();
  device().graphicQueue()->submitCommands(std::move(commandProducer));
}

void TestWindow::guiImplementation()
{
  GUIWindow::guiImplementation();

  _cameraManipulator.update(ImVec2(0.0f, 0.0f),
                            ImVec2((float)size().x, (float)size().y));

  _frameBuilder.makeGui();
}
