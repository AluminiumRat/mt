#include <yaml-cpp/yaml.h>

#include <technique/Technique.h>
#include <techniquePropertySet/TechniquePropertySet.h>
#include <util/Assert.h>

namespace fs = std::filesystem;

using namespace mt;

TechniquePropertySet::TechniquePropertySet(
                                    mt::Technique& technique,
                                    TextureManager& textureManager,
                                    BufferResourceManager& bufferManager,
                                    CommandQueueTransfer& resourceOwnerQueue) :
  _technique(technique),
  _lastConfigurationRevision(0),
  _commonData{.textureManager = &textureManager,
              .bufferManager  = &bufferManager,
              .resourceOwnerQueue = &resourceOwnerQueue}
{
  updateFromTechnique();
}

void TechniquePropertySet::updateFromTechnique()
{
  const mt::TechniqueConfiguration* configuration = _technique.configuration();
  if( configuration == nullptr ||
      _lastConfigurationRevision == configuration->revision)
  {
    return;
  }

  //  Обновляем все старые проперти
  for(PropertiesMap::iterator iProperty = _properties.begin();
      iProperty != _properties.end();
      iProperty++)
  {
    iProperty->second->updateFromTechnique();
  }

  //  Добавляем проперти для вновь появившихся свойств

  // Проперти для юниформов
  for(const mt::TechniqueConfiguration::UniformBuffer& buffer :
                                                configuration->uniformBuffers)
  {
    for(const mt::TechniqueConfiguration::UniformVariable& variable :
                                                            buffer.variables)
    {
      _addProperty(variable.fullName, variable.shortName);
    }
  }

  // Проперти для ресурсов
  for(const mt::TechniqueConfiguration::Resource& resource :
                                                    configuration->resources)
  {
    if(resource.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) continue;
    _addProperty(resource.name, resource.name);
  }

  _lastConfigurationRevision = configuration->revision;
}

void TechniquePropertySet::_addProperty(const std::string& fullName,
                                        const std::string& shortName)
{
  PropertiesMap::const_iterator iProperty = _properties.find(fullName);
  if(iProperty != _properties.end()) return;

  std::unique_ptr<TechniqueProperty> newProperty(
                                            new TechniqueProperty(_technique,
                                                                  fullName,
                                                                  shortName,
                                                                  _commonData));
  _properties[fullName] = std::move(newProperty);
}

void TechniquePropertySet::save(YAML::Emitter& target,
                                const fs::path& resourcesRootFolder) const
{
  target << YAML::BeginSeq;

  for ( PropertiesMap::const_iterator iProperty = _properties.begin();
        iProperty != _properties.end();
        iProperty++)
  {
    const TechniqueProperty* property = iProperty->second.get();
    if(property->isActive()) property->save(target, resourcesRootFolder);
  }

  target << YAML::EndSeq;
}

void TechniquePropertySet::load(const YAML::Node& source,
                                const fs::path& resourcesRootFolder)
{
  if(!source.IsDefined() || !source.IsSequence()) return;

  //  Грузим настройки техники
  for(YAML::Node propertyNode : source)
  {
    //  Нам нужно полное и короткое имя, прежде чем мы сможем найти или создать
    //  настройку
    std::string fullPropertyName =
                                  TechniqueProperty::readFullName(propertyNode);
    if(fullPropertyName.empty()) continue;

    std::string shortPropertyName =
                                TechniqueProperty::readShortName(propertyNode);
    if (shortPropertyName.empty()) shortPropertyName = fullPropertyName;

    //  Если настройки ещё нет, то добавим её
    _addProperty(fullPropertyName, shortPropertyName);

    //  Найдем и загрузим настройку
    PropertiesMap::iterator iProperty = _properties.find(fullPropertyName);
    MT_ASSERT(iProperty != _properties.end());
    iProperty->second->load(propertyNode, resourcesRootFolder);
  }
}
