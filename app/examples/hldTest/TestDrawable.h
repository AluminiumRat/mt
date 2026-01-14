#pragma once

#include <hld/drawScene/Drawable.h>
#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class CommandProducerGraphic;
  class FrameContext;
  class DrawCommandList;

  class TestDrawable : public Drawable
  {
  public:
    TestDrawable(Device& device, float distance);
    TestDrawable(const TestDrawable&) = delete;
    TestDrawable& operator = (const TestDrawable&) = delete;
    virtual ~TestDrawable() noexcept = default;

    virtual void addToDrawPlan( DrawPlan& plan,
                                uint32_t frameTypeIndex) const override;
    void addToCommandList(DrawCommandList& commandList,
                          const FrameContext& frameContext) const;
    void draw(CommandProducerGraphic& commandProducer) const;

  private:
    void _createVertexBuffer(Device& device);
    void _createTexture(Device& device);

  private:
    float _distance;

    uint32_t _colorFrameType;
    uint32_t _drawStage;
    uint32_t _drawCommandGroup;

    Ref<TechniqueConfigurator> _configurator;
    Ref<Technique> _technique;
    TechniquePass& _pass;
    ResourceBinding& _vertexBuffer;
    ResourceBinding& _texture;
  };
}