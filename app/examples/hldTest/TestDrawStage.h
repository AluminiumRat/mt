#pragma once

#include <cstdint>

namespace mt
{
  class TestDrawStage
  {
  public:
    static constexpr const char* stageName = "TestDrawStage";

  public:
    TestDrawStage();
    TestDrawStage(const TestDrawStage&) = delete;
    TestDrawStage& operator = (const TestDrawStage&) = delete;
    ~TestDrawStage() noexcept = default;

    void draw(FrameContext& frameContext) const;

  private:
    uint32_t _stageIndex;
  };
}