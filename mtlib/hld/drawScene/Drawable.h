#pragma once

#include <hld/FrameTypeIndex.h>

namespace mt
{
  class Drawable3D;
  class DrawCommandList;
  class DrawPlan;
  class FrameContext;

  //  Какой-то объект, который может быть добавлен в план по отрисовке кадра а
  //    после обработан на одной или нескольких стадиях рендера
  class Drawable
  {
  public:
    //  Каким образом стади отрисовки должна обрабатывать этот Drawable
    enum DrawType
    {
      //  Отрисовка производится вызовом метода draw.
      //  Поведение метода addToCommandList не определено
      DIRECT_DRAW,
      //  Drawable создает команды для отрисовки с помощью метода
      //    addToCommandList
      //  Поведение метода draw не определено
      COMMANDS_DRAW,
      //  Кастомный способ отрисовки, привязанный к стадии.
      //  Стадия кастит указатель на Drawable к нужному классу. Логика рендера
      //    зашита в стадии и в кастомном Drawable
      CUSTOM_DRAW
    };

  public:
    explicit Drawable(DrawType drawType) noexcept;
    Drawable(const Drawable&) = delete;
    Drawable& operator = (const Drawable&) = delete;
    virtual ~Drawable() noexcept = default;

    inline DrawType drawType() const noexcept;

    virtual void addToDrawPlan( DrawPlan& plan,
                                FrameTypeIndex frameTypeIndex) const = 0;

    //  Использовать commandProducer из frameContext и добавить в него
    //    команды на отрисовку.
    //  Метод используется, если drawType = DIRECT_DRAW. В противном
    //    случае поведение метода не определено
    virtual void draw(const FrameContext& frameContext) const;

    //  Добавить в commandList команды рисования для конкретного frameType
    //    и конкретной стадии. frameTypeIndex и drawStageIndex указаны в
    //    frameContext
    //  Метод используется, если drawType = COMMANDS_DRAW. В противном
    //    случае поведение метода не определено
    virtual void addToCommandList(DrawCommandList& commandList,
                                  const FrameContext& frameContext) const;

    virtual Drawable3D* asDrawable3D() noexcept;
    virtual const Drawable3D* asDrawable3D() const noexcept;

  private:
    DrawType _drawType;
  };

  inline Drawable::DrawType Drawable::drawType() const noexcept
  {
    return _drawType;
  }
};