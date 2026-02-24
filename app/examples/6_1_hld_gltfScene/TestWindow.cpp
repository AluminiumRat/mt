#include <glm/gtc/matrix_transform.hpp>

#include <gltfSupport/GLTFImporter.h>
#include <gui/ImGuiRAII.h>
#include <gui/modalDialogs.h>
#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <imageIO/imageIO.h>
#include <technique/TechniqueLoader.h>
#include <vkr/Device.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  GUIWindow(device,
            "Test window",
            VK_PRESENT_MODE_FIFO_KHR,
            std::nullopt,
            VK_FORMAT_UNDEFINED),
  _asyncQueue(( [&](const mt::AsyncTaskQueue::Event& theEvent)
                {
                  _asyncTaskGui.addEvent(theEvent);
                })),
  _textureManager(_fileWatcher, _asyncQueue),
  _techniqueManager(_fileWatcher, _asyncQueue),
  _frameBuilder(device, _textureManager, _techniqueManager),
  _cameraManipulator(CameraManipulator::APPLICATION_WINDOW_LOCATION),
  _environment(device, _textureManager)
{
  setNeedClearFrameBuffer(false);

  _cameraManipulator.setCamera(&_camera);

  //_loadModel("examples/Box/glTF/Box.gltf");
  _loadModel("examples/Duck/glTF/Duck.gltf");

  _environment.setDirectLightIrradiance(glm::vec3(0,0,0));
  _environment.setIBLMaps("environment/monochrome_studio_03/irradiance_CUBEMAP.dds",
                          "environment/monochrome_studio_03/specular_CUBEMAP.dds");
  //_environment.setIBLMaps("environment/sunny_rose_garden/irradiance_CUBEMAP.dds",
  //                        "environment/sunny_rose_garden/specular_CUBEMAP.dds");
}

void TestWindow::_loadModel(const std::filesystem::path& filename)
{
  _clearScene();

  GLTFImporter importer(*device().graphicQueue(),
                        _textureManager,
                        _techniqueManager);
  std::vector<std::unique_ptr<MeshDrawable>> meshes =
                                                  importer.importGLTF(filename);
  for(std::unique_ptr<MeshDrawable>& mesh : meshes)
  {
    _drawables.push_back(std::move(mesh));
    _scene.registerDrawable(*_drawables.back());
  }
}

void TestWindow::_clearScene() noexcept
{
  for(std::unique_ptr<Drawable>& drawable : _drawables)
  {
    _scene.unregisterDrawable(*drawable);
  }
  _drawables.clear();
}

void TestWindow::update()
{
  _asyncQueue.update();
  _fileWatcher.propagateChanges();

  GUIWindow::update();
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

  _makeMainMenu();

  _frameBuilder.makeGui();
  _asyncTaskGui.makeImGUI();
}

void TestWindow::_makeMainMenu()
{
  mt::ImGuiMainMenuBar mainMenu;
  if(!mainMenu.created()) return;

  if(ImGui::MenuItem("Open model", "Ctrl+O"))
  {
    std::filesystem::path file =
                openFileDialog( this,
                                FileFilters{{ .expression = "*.gltf",
                                              .description = "gltf(*.gltf)"}},
                                "");
    if(!file.empty()) _loadModel(file);
  }
}
