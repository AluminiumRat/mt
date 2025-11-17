#include <technique/Technique.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

Technique::Technique(Device& device, const char* debugName) :
  _device(device),
  _debugName(debugName),
  _isBinded(false),
  _configurator(new TechniqueConfigurator(
                                      device,
                                      (_debugName + "Configurator").c_str())),
  _lastConfiguratorRevision(0)
{
}

bool Technique::bindGraphic(CommandProducerGraphic& producer)
{
  MT_ASSERT(!_isBinded);

  _checkConfiguration();
  if(_configuration == nullptr)
  {
    Log::warning() << _debugName << ": configuration is invalid";
    return false;
  }
  MT_ASSERT(_configurator->pipelineType() == AbstractPipeline::GRAPHIC_PIPELINE);

  _isBinded = true;
  return true;
}

void Technique::_checkConfiguration()
{
  if(_lastConfiguratorRevision != _configurator->revision())
  {
    _lastConfiguratorRevision = _configurator->revision();
    _configuration = Ref(_configurator->configuration());
    if(_configuration != nullptr)
    {
      try
      {
        _applyToConfiguration();
      }
      catch(...)
      {
        _configuration.reset();
        throw;
      }
    }
  }
}

void Technique::_applyToConfiguration()
{
}

void Technique::unbindGraphic(CommandProducerGraphic& producer)
{
  MT_ASSERT(_isBinded);
  _isBinded = false;
}
