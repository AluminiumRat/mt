#include <stdexcept>

#include <technique/Technique.h>
#include <util/Abort.h>
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
  _lastConfiguratorRevision(0),
  _pipelineVariant(0),
  _selectionsRevision(0),
  _lastProcessedSelectionsRevision(0)
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

  _checkSelections();
  MT_ASSERT(_pipelineVariant < _configuration->graphicPipelineVariants.size());

  producer.setGraphicPipeline(
                    *_configuration->graphicPipelineVariants[_pipelineVariant]);

  _isBinded = true;
  return true;
}

void Technique::_checkConfiguration()
{
  if(_lastConfiguratorRevision != _configurator->revision())
  {
    _lastConfiguratorRevision = _configurator->revision();
    const TechniqueConfiguration* newConfiguration =
                                                _configurator->configuration();
    try
    {
      for(std::unique_ptr<SelectionImpl>& selection : _selections)
      {
        selection->setConfiguration(newConfiguration);
      }
      _configuration = ConstRef(newConfiguration);
    }
    catch(std::exception& error)
    {
      Log::error() << _debugName << ": unable to update configuration: " << error.what();
      Abort("Unable to update technique configuration");
    }
  }
}

void Technique::_checkSelections() noexcept
{
  if(_lastProcessedSelectionsRevision != _selectionsRevision)
  {
    _pipelineVariant = 0;
    for (std::unique_ptr<SelectionImpl>& selection : _selections)
    {
      _pipelineVariant += selection->valueWeight();
    }
    _lastProcessedSelectionsRevision = _selectionsRevision;
  }
}

void Technique::unbindGraphic(CommandProducerGraphic& producer)
{
  MT_ASSERT(_isBinded);
  _isBinded = false;
}

Selection& Technique::getOrCreateSelection(const char* selectionName)
{
  for(std::unique_ptr<SelectionImpl>& selection : _selections)
  {
    if(selection->name() == selectionName) return *selection;
  }
  _selections.push_back(std::make_unique<SelectionImpl>(selectionName,
                                                        _selectionsRevision,
                                                        _configuration.get()));
  _selectionsRevision++;
  return *_selections.back();
}
