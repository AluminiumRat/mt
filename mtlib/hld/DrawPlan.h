#pragma once

#include <vector>

namespace mt
{
  class Drawable;

  class DrawPlan
  {
  public:
    using StagePlan = std::vector<Drawable*>;

  public:
    DrawPlan() = default;
    DrawPlan(const DrawPlan&) = default;
    DrawPlan(DrawPlan&&) noexcept = default;
    DrawPlan& operator = (const DrawPlan&) = default;
    DrawPlan& operator = (DrawPlan&&) noexcept = default;
    ~DrawPlan() noexcept = default;

    inline void addDrawable(Drawable& drawable, uint32_t stageIndex);
    inline const StagePlan& stagePlan(uint32_t stageIndex) const noexcept;
    inline void clear() noexcept;

  private:
    using Stages = std::vector<StagePlan>;
    Stages _stages;

    StagePlan _emptyStagePlan;
  };

  inline void DrawPlan::addDrawable(Drawable& drawable, uint32_t stageIndex)
  {
  }

  inline const DrawPlan::StagePlan& DrawPlan::stagePlan(
                                              uint32_t stageIndex) const noexcept
  {
    if( stageIndex >= _stages.size()) return _emptyStagePlan;
    return _stages[stageIndex];
  }

  inline void DrawPlan::clear() noexcept
  {
    for(StagePlan& stage : _stages) stage.clear();
  }
}