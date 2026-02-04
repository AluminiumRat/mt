#pragma once

#include <utility>
#include <vector>

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/DrawPlan.h>
#include <hld/StageIndex.h>

namespace mt
{
  class CommandProducerGraphic;
  struct FrameBuildContext;

  class DrawCommandList
  {
  public:
    //  Сортировка команд перед отрисовкой
    enum Sorting
    {
      NEAR_FIRST_SORTING,     //  Сначала рисуются ближние объекты
      FAR_FIRST_SORTING,      //  Сначала рисуются дальние объекты
      BY_GROUP_INDEX_SORTING  //  Группируем по DrawCommand::groupIndex
    };

  public:
    explicit DrawCommandList(CommandMemoryPool& memoryPool);
    DrawCommandList(const DrawCommandList&) = delete;
    DrawCommandList(DrawCommandList&&) noexcept = default;
    DrawCommandList& operator = (const DrawCommandList&) = delete;
    DrawCommandList& operator = (DrawCommandList&&) noexcept = default;
    ~DrawCommandList() noexcept = default;

    template<typename CommandType, typename... Args>
    inline CommandType& createCommand(Args&&... args);

    inline bool empty() const noexcept;
    inline size_t size() const noexcept;
    inline void clear() noexcept;

    //  Создать для рендер-плана отдельной стадии команды и добавить их
    //  в список.
    //  extraData - дополнительные данные, необходимые для специализированных
    //    стадий.
    void fillFromStagePlan( const DrawPlan::StagePlan& stagePlan,
                            const FrameBuildContext& frameContext,
                            StageIndex stageIndex,
                            const void* extraData);
    void draw(CommandProducerGraphic& producer, Sorting sortingType);

  private:
    void _sort(Sorting sortingType);

  private:
    using Commands = std::vector<CommandPtr>;
    Commands _commands;

    CommandMemoryPool* _memoryPool;
  };

  template<typename CommandType, typename... Args>
  inline CommandType& DrawCommandList::createCommand(Args&&... args)
  {
    CommandPtr newCommand =
              _memoryPool->emplace<CommandType>(std::forward<Args>(args)...);
    CommandType& newCommandRef = static_cast<CommandType&>(*newCommand);
    _commands.emplace_back(std::move(newCommand));
    return newCommandRef;
  }

  inline bool DrawCommandList::empty() const noexcept
  {
    return _commands.empty();
  }

  inline size_t DrawCommandList::size() const noexcept
  {
    return _commands.size();
  }

  inline void DrawCommandList::clear() noexcept
  {
    _commands.clear();
  }
}