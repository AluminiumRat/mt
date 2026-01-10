#include <technique/TechniqueConfiguration.h>
#include <technique/TechniquePass.h>

using namespace mt;

TechniquePass::TechniquePass( const char* name,
                              const TechniqueConfiguration* configuration) :
  _name(name),
  _passIndex(TechniquePassImpl::NOT_PASS_INDEX)
{
  _bindToConfiguration(configuration);
}

void TechniquePass::_bindToConfiguration(
                                    const TechniqueConfiguration* configuration)
{
  if(configuration != nullptr)
  {
    for(_passIndex = 0;
        _passIndex < configuration->_passes.size();
        _passIndex++)
    {
      if(configuration->_passes[_passIndex].name == _name) return;
    }
  }
  _passIndex = TechniquePassImpl::NOT_PASS_INDEX;
}
