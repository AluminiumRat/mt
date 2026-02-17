#include <glm/gtc/matrix_transform.hpp>

#include <gltfSupport/GLTFImporter.h>
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
  _asyncQueue(( [&](const mt::AsyncTaskQueue::Event& theEvent)
                {
                  _asyncTaskGui.addEvent(theEvent);
                })),
  _textureManager(_fileWatcher, _asyncQueue),
  _techniqueManager(_fileWatcher, _asyncQueue),
  _frameBuilder(device),
  _cameraManipulator(CameraManipulator::APPLICATION_WINDOW_LOCATION),
  _environment(device)
{
  setNeedClearFrameBuffer(false);

  _cameraManipulator.setCamera(&_camera);

  _fillScene();
}

void TestWindow::_fillScene()
{
  GLTFImporter importer(*device().graphicQueue(),
                        _textureManager,
                        _techniqueManager);
  std::vector<std::unique_ptr<MeshDrawable>> meshes = importer.importGLTF(
                                                "examples/Duck/glTF/Duck.gltf");
                                                //"examples/Box/glTF/Box.gltf");
  for(std::unique_ptr<MeshDrawable>& mesh : meshes)
  {
    _drawables.push_back(std::move(mesh));
    _scene.registerDrawable(*_drawables.back());
  }
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

  _frameBuilder.makeGui();
  _asyncTaskGui.makeImGUI();
}
