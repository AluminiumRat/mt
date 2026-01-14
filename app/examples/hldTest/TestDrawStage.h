#pragma once

#include <hld/DrawStage.h>

namespace mt
{
  class TestDrawStage : public DrawStage
  {
  public:
    static constexpr const char* stageName = "TestDrawStage";

  public:
    TestDrawStage();
    TestDrawStage(const TestDrawStage&) = delete;
    TestDrawStage& operator = (const TestDrawStage&) = delete;
    virtual ~TestDrawStage() noexcept = default;

  protected:
    virtual void drawImplementation(FrameContext& frameContext) const override;
  };
}