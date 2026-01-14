#include <algorithm>

#include <hld/drawCommand/DrawCommandList.h>

using namespace mt;

DrawCommandList::DrawCommandList(CommandMemoryPool& memoryPool) :
  _memoryPool(&memoryPool)
{
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
    uint32_t chunkGoupIndex = (*chunkStartCommand)->groupIndex();

    Commands::iterator nextCommand = chunkStartCommand;
    nextCommand++;
    while(nextCommand != _commands.end() &&
          (*nextCommand)->groupIndex() == chunkGoupIndex)
    {
      nextCommand++;
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
                else return x->groupIndex() < y->groupIndex();
              });
    break;
  }
}
