#include <cstring>

#include <technique/SpecializationConstantSet.h>
#include <util/Abort.h>
#include <util/Assert.h>

using namespace mt;

void SpecializationConstantSet::_addConstant( const char* name,
                                              const void* data,
                                              uint32_t dataSize)
{
  MT_ASSERT(dataSize != 0);
  MT_ASSERT(data != nullptr);

  for(const ConstantRecord& record : _constantMap)
  {
    if(record.name == name) Abort("SpecializationConstantSet: duplicated constant name");
  }

  ConstantRecord newRecord{ .name = name,
                            .shift = (uint32_t)_dataContainer.size(),
                            .size = dataSize};

  _dataContainer.resize(_dataContainer.size() + dataSize);
  memcpy( _dataContainer.data() + newRecord.shift,
          data,
          dataSize);

  _constantMap.push_back(newRecord);
}
