#include <hld/drawCommand/CommandMemoryPool.h>

using namespace mt;

CommandMemoryPool::CommandMemoryPool(size_t chunkSize) :
  _holderSize(chunkSize),
  _currentHolderIndex(0),
  _currentHolder(nullptr)
{
}

void CommandMemoryPool::reset() noexcept
{
  if(_holders.empty()) return;
  for(size_t iHolder = 0; iHolder < _holders.size(); iHolder++)
  {
    _holders[iHolder]->reset();
  }
  _selectHolder(0);
}

void CommandMemoryPool::_addHolder()
{
  std::unique_ptr<CommandMemoryHolder>
                                newHolder(new CommandMemoryHolder(_holderSize));
  _holders.push_back(std::move(newHolder));
}

void CommandMemoryPool::_selectHolder(size_t holderIndex) noexcept
{
  _currentHolder = _holders[holderIndex].get();
  _currentHolderIndex = holderIndex;
}
