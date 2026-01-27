#pragma once

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Camera;
  class CommandProducerGraphic;
  class DrawPlan;

  class TestDrawStage
  {
  public:
    static constexpr const char* stageName = "TestDrawStage";

  public:
    explicit TestDrawStage(Device& device);
    TestDrawStage(const TestDrawStage&) = delete;
    TestDrawStage& operator = (const TestDrawStage&) = delete;
    ~TestDrawStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              const DrawPlan& drawPlan,
              const FrameBuildContext& frameContext);

    //  Добавить в текущий контекст ImGui виджеты со статистикой последнего кадра
    void makeImGui() const;

  private:
    void _createCommonSet(Device& device);
    void _updateCommonSet(CommandProducerGraphic& commandProducer,
                          const Camera& camera);
    void _processDrawables( CommandProducerGraphic& commandProducer,
                            const DrawPlan& drawPlan,
                            const FrameBuildContext& frameContext);

  private:
    StageIndex _stageIndex;

    Ref<DescriptorSet> _commonDescriptorSet;
    Ref<DataBuffer> _commonUniformBuffer;
    Ref<PipelineLayout> _pipelineLayout;

    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;

    size_t _lastFrameCommandsCount;
  };
}