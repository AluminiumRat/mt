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
  class CommandProducerGraphic;

  class TestDrawStage
  {
  public:
    static constexpr const char* stageName = "TestDrawStage";

  public:
    TestDrawStage(Device& device, FrameTypeIndex frameTypeIndex);
    TestDrawStage(const TestDrawStage&) = delete;
    TestDrawStage& operator = (const TestDrawStage&) = delete;
    ~TestDrawStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              FrameContext& frameContext);

    //  Добавить в текущий контекст ImGui виджеты со статистикой последнего кадра
    void makeImGui() const;

  private:
    void _createCommonSet(Device& device);
    void _updateCommonSet(CommandProducerGraphic& commandProducer,
                          FrameContext& frameContext);
    void _processDrawables( CommandProducerGraphic& commandProducer,
                            FrameContext& frameContext);

  private:
    StageIndex _stageIndex;
    FrameTypeIndex _frameTypeIndex;

    Ref<DescriptorSet> _commonDescriptorSet;
    Ref<DataBuffer> _commonUniformBuffer;
    Ref<PipelineLayout> _pipelineLayout;

    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;

    size_t _lastFrameCommandsCount;
  };
}