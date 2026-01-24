#pragma once

#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>

namespace mt
{
  class CommandProducerGraphic;
  class Drawable3D;
  class DrawCommandList;
  class DrawPlan;

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

    //  Отрисовать объект непосредственно в CommandProducerGraphic
    //  Можно использовать, когда не нужна группировка и сортировка команд
    //    отрисовки
    //  Метод используется, если drawType = DIRECT_DRAW. В противном
    //    случае поведение метода не определено
    //  extraData - дополнительные данные для специализированных стадий.
    virtual void draw(CommandProducerGraphic& commandProducer,
                      FrameTypeIndex frame,
                      StageIndex stage,
                      const void* extraData) const;

    //  Добавить в commandList команды рисования для конкретного frameType
    //    и конкретной стадии
    //  Используется при батчинге (сортировка и группировка команд отрисовки)
    //  Метод используется, если drawType = COMMANDS_DRAW. В противном
    //    случае поведение метода не определено
    //  extraData - дополнительные данные для специализированных стадий.
    virtual void addToCommandList(DrawCommandList& commandList,
                                  FrameTypeIndex frame,
                                  StageIndex stage,
                                  const void* extraData) const;

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