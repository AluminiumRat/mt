#pragma once

#include <cstdint>
#include <memory>
#include <span>

namespace mt
{
  class CommandProducerGraphic;
  class CommandPtr;

  //  Объект - функтор, который рисует что-нибудь
  //  Нужен для того чтобы иметь возможность сортировать и группировать
  //    отрисовку объектов сцены.
  class DrawCommand
  {
  public:
    //  Если использовать это число как groupIndex для DrawCommand, то эти
    //  команды не будут группироваться и всегда будут исполняться по одной
    static constexpr uint32_t noGroupIndex = 0;

  public:
    //  groupIndex - признак, по которому команды могут быть собраны вместе и
    //    выполнены совместно(инстансинг)
    //  distance - расстояние до рисуемого объекта. Используется для сортировки
    //    команд
    //  layer - слой рисования. Используется при сортировке. Команды из разных
    //    слоем не могут смешиваться друг с другом. Слои отрисовываются от
    //    меньшего индекса к большему
    inline DrawCommand( uint32_t groupIndex,
                        int32_t layer,
                        float distance) noexcept;
    DrawCommand(const DrawCommand&) = delete;
    DrawCommand& operator = (const DrawCommand&) = delete;
    virtual ~DrawCommand() noexcept = default;

    inline uint32_t groupIndex() const noexcept;
    inline int32_t layer() const noexcept;
    inline float distance() const noexcept;

    //  список commands используется для совместного выполнения нескольких
    //    команд (например, инстансинг) и содержит хотябы одну команду. Первая
    //    команда в commands - это всегда this. Команды в commands всегда имеют
    //    один и тот же groupIndex
    virtual void draw(CommandProducerGraphic& producer,
                      std::span<const CommandPtr> commands) = 0;
  private:
    uint32_t _groupIndex;
    int32_t _layer;
    float _distance;
  };

  //  Кастомный деструктор для умного указателя CommandPtr. Уничтожает объект
  //    класса DrawCommand, но не освобождает память
  //  Используется для размещения команд в CommandMemoryHolder и
  //    CommandMemoryPool
  class CommandDestructor
  {
  public:
    CommandDestructor() noexcept = default;
    CommandDestructor(const CommandDestructor&) = default;
    CommandDestructor& operator = (const CommandDestructor&) = default;
    ~CommandDestructor() noexcept = default;

    inline void operator() (DrawCommand* command) const noexcept;
  };

  //  Кастомный unique_ptr на DrawCommand
  //  При удалении вызывает деструктор DrawCommand, но не освобождает память,
  //    так как управление памятью происходит централизованно через
  //    CommandMemoryHolder
  //  Используется для размещения команд в CommandMemoryHolder и
  //    CommandMemoryPool
  class CommandPtr : public std::unique_ptr<DrawCommand, CommandDestructor>
  {
  public:
    CommandPtr() noexcept = default;
    explicit inline CommandPtr(DrawCommand* p) noexcept;
    CommandPtr(const CommandPtr&) = delete;
    inline CommandPtr(CommandPtr&& other) noexcept;
    CommandPtr& operator = (const CommandPtr&) = delete;
    inline CommandPtr&  operator = (CommandPtr&& other) noexcept;
    ~CommandPtr() noexcept = default;
  };

  inline DrawCommand::DrawCommand(uint32_t groupIndex,
                                  int32_t layer,
                                  float distance) noexcept :
    _groupIndex(groupIndex),
    _layer(layer),
    _distance(distance)
  {
  }

  inline uint32_t DrawCommand::groupIndex() const noexcept
  {
    return _groupIndex;
  }

  inline int32_t DrawCommand::layer() const noexcept
  {
    return _layer;
  }

  inline float DrawCommand::distance() const noexcept
  {
    return _distance;
  }

  void CommandDestructor::operator()(DrawCommand* resource) const noexcept
  {
    resource->~DrawCommand();
  }

  CommandPtr::CommandPtr(DrawCommand* p) noexcept:
    std::unique_ptr<DrawCommand, CommandDestructor>(p)
  {
  }

  inline CommandPtr::CommandPtr(CommandPtr&& other) noexcept :
    std::unique_ptr<DrawCommand, CommandDestructor>(std::move(other))
  {
  }

  inline CommandPtr& CommandPtr::operator = (CommandPtr&& other) noexcept
  {
    std::unique_ptr<DrawCommand, CommandDestructor>::operator=(
                                                              std::move(other));
    return *this;
  }
}