#include <stdexcept>
#include <technique/Technique.h>
#include <technique/UniformVariable.h>
#include <util/Abort.h>
#include <util/Log.h>

using namespace mt;

UniformVariable::UniformVariable(
            const char* name,
            const Technique& technique,
            const TechniqueConfiguration* configuration,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks) :
  _technique(technique),
  _description(nullptr),
  _valueIsSetted(false),
  _storage(nullptr),
  _savedName(name)
{
  _bindToConfiguration(configuration, blocks);
}

void UniformVariable::_bindToConfiguration(
              const TechniqueConfiguration* configuration,
              const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks)
{
  UniformDescriptions descriptions = _findDescriptions(configuration);
  TechniqueUniformBlock* newStorage = _findStorage(descriptions, blocks);

  const std::string& nameRef = name();
  ValueRef valueRef = getValue();

  try
  {
    // Переносим имя
    if(descriptions.variableDescription == nullptr) _savedName = nameRef;
    else _savedName = std::string();

    // Переносим значение
    if(valueRef.data != nullptr)
    {
      if( newStorage != nullptr &&
          valueRef.dataSize <= descriptions.variableDescription->size)
      {
        newStorage->setData(descriptions.variableDescription->offsetInBuffer,
                            descriptions.variableDescription->size,
                            valueRef.data);
        _savedValue = std::vector<char>();
      }
      else
      {
        _saveValue(valueRef);
      }
    }
  }
  catch(std::exception& error)
  {
    Log::error() << _technique.debugName() << " : " << nameRef << " : " << error.what();
    Abort("Unable to bind to technique configuration");
  }

  _description = descriptions.variableDescription;
  _storage = newStorage;
}

UniformVariable::UniformDescriptions UniformVariable::_findDescriptions(
                              const TechniqueConfiguration* configuration) const
{
  UniformDescriptions descriptions{};
  if(configuration == nullptr) return descriptions;

  const std::string variableName = name();

  for(const TechniqueConfiguration::UniformBuffer& bufferDescription :
                                                  configuration->uniformBuffers)
  {
    for(const TechniqueConfiguration::UniformVariable& variableDescription:
                                                    bufferDescription.variables)
    {
      if(variableDescription.fullName == variableName)
      {
        descriptions.bufferDescription = &bufferDescription;
        descriptions.variableDescription = &variableDescription;
        return descriptions;
      }
    }
  }

  return descriptions;
}

TechniqueUniformBlock* UniformVariable::_findStorage(
            UniformDescriptions descriptions,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& storages)
{
  if(descriptions.bufferDescription == nullptr) return nullptr;
  for(const std::unique_ptr<TechniqueUniformBlock>& block : storages)
  {
    if( block->description().set == descriptions.bufferDescription->set &&
        block->description().binding == descriptions.bufferDescription->binding)
    {
      return block.get();
    }
  }
  return nullptr;
}

void UniformVariable::_saveValue(ValueRef data)
{
  MT_ASSERT(data.data != nullptr && data.dataSize != 0);
  _savedValue.resize(data.dataSize);
  memcpy(_savedValue.data(), data.data, data.dataSize);
}

void UniformVariable::setValue(ValueRef newValue)
{
  if(newValue.data == nullptr)
  {
    _savedValue = std::vector<char>();
    _valueIsSetted = false;
  }
  else
  {
    if( _storage != nullptr &&
        newValue.dataSize <= _description->size)
    {
      _storage->setData(_description->offsetInBuffer,
                        _description->size,
                        newValue.data);
      _savedValue = std::vector<char>();
    }
    else
    {
      _saveValue(newValue);
    }
    _valueIsSetted = true;
  }
}
