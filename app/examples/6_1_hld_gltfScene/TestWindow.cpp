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
            VkSurfaceFormatKHR{ ColorFrameBuilder::frameFormat,
                                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
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

  _loadModel("examples/Duck/glTF/Duck.gltf");
}

void TestWindow::_loadModel(const std::filesystem::path& filename)
{
  _clearScene();

  GLTFImporter importer(*device().graphicQueue(),
                        _textureManager,
                        _techniqueManager,
                        GLTFImporter::LOAD_ASYNC,
                        true);
  GLTFImporter::Results imported = importer.importGLTF(filename);
  for(std::unique_ptr<MeshDrawable>& mesh : imported.drawables)
  {
    _drawables.push_back(std::move(mesh));
    _scene.registerDrawable(*_drawables.back());
  }
  for(std::unique_ptr<BLASInstance>& blas : imported.blases)
  {
    _blases.push_back(std::move(blas));
    _scene.registerBLAS(*_blases.back());
  }
}

void TestWindow::_loadEnvironment(const std::filesystem::path& filename)
{
  _environmentFile.clear();
  _environment.load(filename);
  _environmentFile = filename;
}

void TestWindow::_clearScene() noexcept
{
  for(std::unique_ptr<Drawable>& drawable : _drawables)
  {
    _scene.unregisterDrawable(*drawable);
  }
  _drawables.clear();

  for(std::unique_ptr<BLASInstance>& blas : _blases)
  {
    _scene.unregisterBLAS(*blas);
  }
  _drawables.clear();
}

void TestWindow::update()
{
  GUIWindow::update();

  _asyncQueue.update();
  _fileWatcher.propagateChanges();

  std::unique_ptr<CommandProducerGraphic> updateProducer =
                                device().graphicQueue()->startCommands("Update");
  _scene.updateGPUData(*updateProducer);
  device().graphicQueue()->submitCommands(std::move(updateProducer));
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  _frameBuilder.draw( frameBuffer,
                      _scene,
                      _camera,
                      _environment);

  std::unique_ptr<CommandProducerGraphic> guiProducer =
                                device().graphicQueue()->startCommands("ImGui");
    CommandProducerGraphic::RenderPass renderPass(*guiProducer, frameBuffer);
    drawGUI(*guiProducer);
    renderPass.endPass();
  device().graphicQueue()->submitCommands(std::move(guiProducer));
}

void TestWindow::guiImplementation()
{
  GUIWindow::guiImplementation();

  _cameraManipulator.update(ImVec2(0.0f, 0.0f),
                            ImVec2((float)size().x, (float)size().y));

  _makeMainMenu();

  _frameBuilder.makeGui();
  _environment.makeGui();
  _asyncTaskGui.makeImGUI();
}

void TestWindow::_makeMainMenu()
{
  mt::ImGuiMainMenuBar mainMenu;
  if(!mainMenu.created()) return;

  //  Загрузка модели
  if(ImGui::MenuItem("Model"))
  {
    std::filesystem::path file =
                openFileDialog( this,
                                FileFilters{{ .expression = "*.gltf",
                                              .description = "gltf(*.gltf)"}},
                                "");
    if(!file.empty()) _loadModel(file);
  }

  //  Сохранение/загрузка окружения
  mt::ImGuiMenu environmentMenu("Environment");
  if(environmentMenu.created())
  {
    if(ImGui::MenuItem("Load"))
    {
      std::filesystem::path file =
                openFileDialog( this,
                                FileFilters{
                                        { .expression = "*.env",
                                          .description = "environment(*.env)"}},
                                "");
      if(!file.empty()) _loadEnvironment(file);
    }

    if(ImGui::MenuItem("Save"))
    {
      if(!_environmentFile.empty())
      {
        _environment.save(_environmentFile);
      }
      else
      {
        errorDialog(this,
                    "Error",
                    "The environment is not loaded properly");
      }
    }

    environmentMenu.end();
  }
}
