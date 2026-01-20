#pragma once

#include <gui/cameraManipulator/OrbitalCameraManipulator.h>
#include <gui/GUIWindow.h>
#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawScene/SimpleDrawScene.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/DrawPlan.h>
#include <hld/FrameTypeIndex.h>
#include <util/Camera.h>
#include <util/Ref.h>

#include <TestDrawStage.h>

namespace mt
{
  class TestWindow : public GUIWindow
  {
  public:
    static constexpr const char* colorFrameType = "ColorFrame";

  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void guiImplementation() override;
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;
  private:
    void _setupMeshAsset();

  private:
    FrameTypeIndex _frameTypeIndex;

    Camera _camera;
    OrbitalCameraManipulator _cameraManipulator;

    Ref<MeshAsset> _meshAsset;

    SimpleDrawScene _scene;
    MeshDrawable _drawable1;
    MeshDrawable _drawable2;
    MeshDrawable _drawable3;

    TestDrawStage _drawStage;

    DrawPlan _drawPlan;
    CommandMemoryPool _commandMemoryPool;
  };
}