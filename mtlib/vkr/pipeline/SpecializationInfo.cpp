#include <cstring>

#include <vkr/pipeline/SpecializationInfo.h>
#include <util/Abort.h>
#include <util/Assert.h>

using namespace mt;

SpecializationInfo::SpecializationInfo() noexcept :
  _vulkanInfo{}
{
}

void SpecializationInfo::addConstant( uint32_t constantID,
                                      const void* data,
                                      size_t dataSize)
{
  MT_ASSERT(data != nullptr);
  MT_ASSERT(dataSize > 0);
  MT_ASSERT(dataSize <= 8);

  for(const VkSpecializationMapEntry& entry : _entryInfo)
  {
    if(entry.constantID == constantID) Abort("SpecializationInfo::addConstant: duplicate specialization constant ID");
  }

  VkSpecializationMapEntry newRecord{ .constantID = constantID,
                                      .offset = uint32_t(_entryData.size()),
                                      .size = dataSize};
  _entryData.resize(_entryData.size() + dataSize);
  memcpy( _entryData.data() + newRecord.offset,
          data,
          dataSize);
  _entryInfo.push_back(newRecord);

  _updateVulkanInfo();
}

void SpecializationInfo::_updateVulkanInfo() noexcept
{
  _vulkanInfo.mapEntryCount = uint32_t(_entryInfo.size());
  _vulkanInfo.pMapEntries = _entryInfo.data();
  _vulkanInfo.dataSize = _entryData.size();
  _vulkanInfo.pData = _entryData.data();
}
