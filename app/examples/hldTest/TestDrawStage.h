#pragma once

#include <cstdint>

#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class TestDrawStage
  {
  public:
    static constexpr const char* stageName = "TestDrawStage";

  public:
    explicit TestDrawStage(Device& device);
    TestDrawStage(const TestDrawStage&) = delete;
    TestDrawStage& operator = (const TestDrawStage&) = delete;
    ~TestDrawStage() noexcept = default;

    void draw(FrameContext& frameContext) const;

    //  Добавить в текущий контекст ImGui виджеты со статистикой последнего кадра
    void makeImGui() const;

  private:
    void _createCommonSet(Device& device);
    void _updateCommonSet(FrameContext& frameContext) const;
    void _processDrawables(FrameContext& frameContext) const;

  private:
    StageIndex _stageIndex;

    Ref<DescriptorSet> _commonDescriptorSet;
    Ref<DataBuffer> _commonUniformBuffer;
    Ref<PipelineLayout> _pipelineLayout;

    mutable size_t _lastFrameCommandsCount;
  };
}