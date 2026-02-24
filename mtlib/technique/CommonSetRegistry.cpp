#include <technique/CommonSetRegistry.h>
#include <util/Assert.h>
#include <util/Log.h>

using namespace mt;

CommonSetRegistry& CommonSetRegistry::_instance()
{
  static CommonSetRegistry registry;
  return registry;
}

void CommonSetRegistry::registerSet(
                        const char* setName,
                        std::span<const VkDescriptorSetLayoutBinding> bindings)
{
  Bindings bindingsCopy(bindings.begin(), bindings.end());

  CommonSetRegistry& registry = _instance();
  auto insertion = registry._sets.insert({setName, std::move(bindingsCopy)});
  if(!insertion.second)
  {
    Log::error() << "CommonSetRegistry::registerSet: duplicate setName " << setName;
    MT_ASSERT("CommonSetRegistry::registerSet: duplicate setName");
  }
}

const std::vector<VkDescriptorSetLayoutBinding>&
                        CommonSetRegistry::getSet(const char* setName) noexcept
{
  CommonSetRegistry& registry = _instance();
  
  Sets::iterator iSet = registry._sets.find(setName);
  if(iSet == registry._sets.end())
  {
    Log::error() << "CommonSetRegistry::registerSet: unregistered set name " << setName;
    MT_ASSERT("CommonSetRegistry::registerSet: unregistered set name");
  }
  return iSet->second;
}
