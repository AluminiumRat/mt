#pragma once

#include <memory>
#include <vector>

#include <asyncTask/AsyncTaskQueue.h>
#include <gui/cameraManipulator/OrbitalCameraManipulator.h>
#include <gui/AsyncTaskGUI.h>
#include <gui/GUIWindow.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <hld/drawScene/DrawScene.h>
#include <resourceManagement/FileWatcher.h>
#include <resourceManagement/TechniqueManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/Camera.h>
#include <util/Ref.h>

namespace mt
{
  class TestWindow : public GUIWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

    virtual void update() override;

  protected:
    virtual void guiImplementation() override;
    virtual void drawImplementation(FrameBuffer& frameBuffer) override;

  private:
    void _fillScene();

  private:
    FileWatcher _fileWatcher;
    AsyncTaskQueue _asyncQueue;
    AsyncTaskGUI _asyncTaskGui;
    TextureManager _textureManager;
    TechniqueManager _techniqueManager;

    ColorFrameBuilder _frameBuilder;

    Camera _camera;
    OrbitalCameraManipulator _cameraManipulator;

    DrawScene _scene;
    std::vector<std::unique_ptr<Drawable>> _drawables;

    EnvironmentScene _environment;
  };
}