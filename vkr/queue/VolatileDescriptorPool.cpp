#include <vkr/queue/VolatileDescriptorPool.h>

using namespace mt;

VolatileDescriptorPool::VolatileDescriptorPool(
                              const DescriptorCounter& initialDescriptorNumber,
                              uint32_t initializeSetNumber,
                              Device& device):
  _device(device),
  _initialDescriptorNumber(initialDescriptorNumber),
  _initialSetNumber(initializeSetNumber),
  _currentPoolIndex(0),
  _currentPool(nullptr)
{
}

VkDescriptorSet VolatileDescriptorPool::allocateSet(
                                    VkDescriptorSetLayout layout,
                                    const DescriptorCounter& descriptorsNumber)
{
  if(_currentPool == nullptr)
  {
    _addPool();
    _selectPool(0);
  }

  if( !_currentPool->descriptorsLeft().contains(descriptorsNumber)||
      _currentPool->setsLeft() == 0)
  {
    if(_currentPoolIndex == _subpools.size() - 1) _addPool();
    _selectPool(_currentPoolIndex + 1);
  }

  return _currentPool->allocateSet(layout, descriptorsNumber);
}

void VolatileDescriptorPool::_addPool()
{
  Ref<DescriptorPool> newPool( new DescriptorPool(_initialDescriptorNumber,
                                                  _initialSetNumber,
                                                  _device));
  _subpools.push_back(std::move(newPool));
}

void VolatileDescriptorPool::_selectPool(size_t poolIndex)
{
  _currentPoolIndex = poolIndex;
  _currentPool = _subpools[poolIndex].get();
}

void VolatileDescriptorPool::reset()
{
  if(_subpools.empty()) return;
  for(size_t poolIndex = 0; poolIndex < _subpools.size(); poolIndex++)
  {
    _subpools[poolIndex]->reset();
  }
  _selectPool(0);
}
