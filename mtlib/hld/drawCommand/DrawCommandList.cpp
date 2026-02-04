#include <algorithm>

#include <hld/drawCommand/DrawCommandList.h>
#include <hld/drawScene/Drawable.h>
#include <util/Assert.h>

using namespace mt;

DrawCommandList::DrawCommandList(CommandMemoryPool& memoryPool) :
  _memoryPool(&memoryPool)
{
}

void DrawCommandList::fillFromStagePlan(const DrawPlan::StagePlan& stagePlan,
                                        const FrameBuildContext& frameContext,
                                        StageIndex stageIndex,
                                        const void* extraData)
{
  for(const Drawable* drawable : stagePlan)
  {
    MT_ASSERT(drawable->drawType() == Drawable::COMMANDS_DRAW);
    drawable->addToCommandList( *this,
                                frameContext,
                                stageIndex,
                                extraData);
  }
}

void DrawCommandList::draw(CommandProducerGraphic& producer, Sorting sortingType)
{
  if(_commands.empty()) return;

  _sort(sortingType);

  //  Отрисовка. Ищем куски последовательно идущих команд с одинаковыми
  //    groupIndex и отрисовываем их вместе
  Commands::iterator chunkStartCommand = _commands.begin();
  while(chunkStartCommand != _commands.end())
  {
    DrawCommand::Group chunkGoup = (*chunkStartCommand)->group();

    Commands::iterator nextCommand = chunkStartCommand;
    nextCommand++;

    if(chunkGoup != DrawCommand::noGroup)
    {
      while(nextCommand != _commands.end() &&
            (*nextCommand)->group() == chunkGoup)
      {
        nextCommand++;
      }
    }

    std::span<const CommandPtr> commandsChunk(&(*chunkStartCommand),
                                              nextCommand - chunkStartCommand);
    (*chunkStartCommand)->draw(producer, commandsChunk);

    chunkStartCommand = nextCommand;
  }
}

void DrawCommandList::_sort(Sorting sortingType)
{
  switch(sortingType)
  {
  case NEAR_FIRST_SORTING:
    std::sort(_commands.begin(),
              _commands.end(),
              [](const CommandPtr& x, const CommandPtr& y) -> bool
              {
                if(x->layer() != y->layer()) return x->layer() > y->layer();
                else return x->distance() < y->distance();
              });
    break;

  case FAR_FIRST_SORTING:
    std::sort(_commands.begin(),
              _commands.end(),
              [](const CommandPtr& x, const CommandPtr& y) -> bool
              {
                if(x->layer() != y->layer()) return x->layer() < y->layer();
                else return x->distance() > y->distance();
              });
    break;

  case BY_GROUP_INDEX_SORTING:
    std::sort(_commands.begin(),
              _commands.end(),
              [](const CommandPtr& x, const CommandPtr& y) -> bool
              {
                if(x->layer() != y->layer()) return x->layer() < y->layer();
                else return x->group() < y->group();
              });
    break;
  }
}
