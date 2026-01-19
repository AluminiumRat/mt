#pragma once

#include <vector>

#include <hld/StageIndex.h>

namespace mt
{
  class Drawable;

  class DrawPlan
  {
  public:
    using StagePlan = std::vector<const Drawable*>;

  public:
    DrawPlan() = default;
    DrawPlan(const DrawPlan&) = default;
    DrawPlan(DrawPlan&&) noexcept = default;
    DrawPlan& operator = (const DrawPlan&) = default;
    DrawPlan& operator = (DrawPlan&&) noexcept = default;
    ~DrawPlan() noexcept = default;

    inline void addDrawable(const Drawable& drawable, StageIndex stageIndex);
    inline const StagePlan& stagePlan(StageIndex stageIndex) const noexcept;
    inline void clear() noexcept;

  private:
    using Stages = std::vector<StagePlan>;
    Stages _stages;

    StagePlan _emptyStagePlan;
  };

  inline void DrawPlan::addDrawable(const Drawable& drawable,
                                    StageIndex stageIndex)
  {
    if (stageIndex >= _stages.size()) _stages.resize(stageIndex + 1);
    _stages[stageIndex].push_back(&drawable);
  }

  inline const DrawPlan::StagePlan& DrawPlan::stagePlan(
                                          StageIndex stageIndex) const noexcept
  {
    if( stageIndex >= _stages.size()) return _emptyStagePlan;
    return _stages[stageIndex];
  }

  inline void DrawPlan::clear() noexcept
  {
    for(StagePlan& stage : _stages) stage.clear();
  }
}