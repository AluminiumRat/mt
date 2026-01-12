#include <util/StringRegistry.h>

using namespace mt;

StringRegistry::StringRegistry() :
  _indexCursor(0)
{
}

size_t StringRegistry::getIndex(const std::string& theString)
{
  std::lock_guard lock(_accessMutex);
  RegistryMap::iterator iString = _map.find(theString);
  if(iString == _map.end())
  {
    _map[theString] = _indexCursor;
    return _indexCursor++;
  }
  else return iString->second;
}
