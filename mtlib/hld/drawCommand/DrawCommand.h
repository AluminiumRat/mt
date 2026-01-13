#pragma once

#include <cstdint>
#include <span>

namespace mt
{
  class CommandProducerGraphic;

  //  Объект - функтор, который рисует что-нибудь
  //  Нужен для того чтобы иметь возможность сортировать и группировать
  //    отрисовку объектов сцены.
  class DrawCommand
  {
  public:
    //  groupIndex - признак, по которому команды могут быть собраны вместе и
    //    выполнены совместно(инстансинг)
    //  distance - расстояние до рисуемого объекта. Используется для сортировки
    //    команд
    //  layer - слой рисования. Используется при сортировке. Команды из разных
    //    слоем не могут смешиваться друг с другом. Слои отрисовываются от
    //    меньшего индекса к большему
    DrawCommand(uint32_t groupIndex, uint32_t layer, float distance) noexcept;
    DrawCommand(const DrawCommand&) = delete;
    DrawCommand& operator = (const DrawCommand&) = delete;
    virtual ~DrawCommand() noexcept = default;

    inline uint32_t groupIndex() const noexcept;
    inline uint32_t layer() const noexcept;
    inline float distance() const noexcept;

    //  this - это первая команда в commands
    virtual void draw(CommandProducerGraphic& producer,
                      std::span<DrawCommand> commands) = 0;

  private:
    uint32_t _groupIndex;
    uint32_t _layer;
    float _distance;
  };

  inline uint32_t DrawCommand::groupIndex() const noexcept
  {
    return _groupIndex;
  }

  inline uint32_t DrawCommand::layer() const noexcept
  {
    return _layer;
  }

  inline float DrawCommand::distance() const noexcept
  {
    return _distance;
  }
}