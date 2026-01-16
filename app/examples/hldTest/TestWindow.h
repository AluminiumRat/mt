#pragma once

#include <gui/RenderWindow.h>
#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawScene/SimpleDrawScene.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/DrawPlan.h>
#include <util/Camera.h>

#include <TestDrawStage.h>

namespace mt
{
  class TestWindow : public RenderWindow
  {
  public:
    static constexpr const char* colorFrameType = "ColorFrame";

  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;
  private:
    void _createMeshAsset();

  private:
    uint32_t _frameTypeIndex;

    Camera _camera;
    SimpleDrawScene _scene;
    MeshDrawable _drawable1;
    MeshDrawable _drawable2;
    MeshDrawable _drawable3;

    TestDrawStage _drawStage;

    DrawPlan _drawPlan;
    CommandMemoryPool _commandMemoryPool;
  };
}