#pragma once

#include <string>

namespace mt
{
  class FrameContext;

  class DrawStage
  {
  public:
    DrawStage(const char* name);
    DrawStage(const DrawStage&) = delete;
    DrawStage& operator = (const DrawStage&) = delete;
    virtual ~DrawStage() noexcept = default;

    inline const std::string& name() const noexcept;
    inline uint32_t stageIndex() const noexcept;

    virtual void draw(FrameContext& frameContext) const;

  protected:
    virtual void setCommonSet(FrameContext& frameContext) const;
    virtual void drawImplementation(FrameContext& frameContext) const;

  private:
    std::string _name;
    uint32_t _stageIndex;
  };

  inline const std::string& DrawStage::name() const noexcept
  {
    return _name;
  }

  inline uint32_t DrawStage::stageIndex() const noexcept
  {
    return _stageIndex;
  }
}