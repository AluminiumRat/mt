#pragma once

#include <cstdint>

namespace mt
{
  class DrawPlan;

  //  Какой-то объект, который может быть добавлен в план по отрисовке кадра
  class Drawable
  {
  public:
    Drawable() noexcept = default;
    Drawable(const Drawable&) = delete;
    Drawable& operator = (const Drawable&) = delete;
    virtual ~Drawable() noexcept = default;

    virtual void addToDrawPlan(DrawPlan& plan, uint32_t frameTypeIndex) = 0;
  };
};