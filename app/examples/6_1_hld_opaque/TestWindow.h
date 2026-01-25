#pragma once

#include <memory>
#include <vector>

#include <gui/cameraManipulator/OrbitalCameraManipulator.h>
#include <gui/GUIWindow.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <hld/meshDrawable/MeshDrawable.h>
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

  protected:
    virtual void guiImplementation() override;
    virtual void drawImplementation(FrameBuffer& frameBuffer) override;
  private:
    void _setupMeshAsset();
    void _fillScene();

  private:
    ColorFrameBuilder _frameBuilder;

    Camera _camera;
    OrbitalCameraManipulator _cameraManipulator;

    Ref<MeshAsset> _meshAsset;

    DrawScene _scene;
    std::vector<std::unique_ptr<MeshDrawable>> _drawables;

    GlobalLight _illumination;
  };
}