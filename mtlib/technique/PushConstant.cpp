#include <technique/PushConstant.h>

using namespace mt;

PushConstant::PushConstant( const char* name,
                            const TechniqueConfiguration* configuration) :
  _name(name),
  _description(nullptr)
{
  _bindToConfiguration(configuration);
}

void PushConstant::_bindToConfiguration(
                                    const TechniqueConfiguration* configuration)
{
  _description = nullptr;
  if (configuration == nullptr) return;

  for(const TechniqueConfiguration::PushConstant& descrition :
                                                  configuration->pushConstants)
  {
    if(descrition.fullName == _name)
    {
      _description = &descrition;
      return;
    }
  }
}