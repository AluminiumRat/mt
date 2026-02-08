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
                                          CommandQueueGraphic& uploadingQueue) :
  _technique(technique),
  _lastConfigurationRevision(0),
  _commonData{.textureManager = &textureManager,
              .bufferManager  = &bufferManager,
              .uploadingQueue = &uploadingQueue}
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
      _addProperty(variable.fullName);
    }
  }

  // Проперти для ресурсов
  for(const mt::TechniqueConfiguration::Resource& resource :
                                                    configuration->resources)
  {
    if(resource.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) continue;
    _addProperty(resource.name);
  }

  _lastConfigurationRevision = configuration->revision;
}

void TechniquePropertySet::_addProperty(const std::string& name)
{
  PropertiesMap::const_iterator iProperty = _properties.find(name);
  if(iProperty != _properties.end()) return;

  std::unique_ptr<TechniqueProperty> newProperty(
                                            new TechniqueProperty(_technique,
                                                                  name,
                                                                  _commonData));
  _properties[name] = std::move(newProperty);
}

void TechniquePropertySet::save(YAML::Emitter& target,
                                const fs::path& resourcesRootFolder) const
{
  target << YAML::BeginMap;

  for ( PropertiesMap::const_iterator iProperty = _properties.begin();
        iProperty != _properties.end();
        iProperty++)
  {
    const TechniqueProperty* property = iProperty->second.get();
    if(property->isActive())
    {
      target << YAML::Key << property->name();
      target << YAML::Value;
      property->save(target, resourcesRootFolder);
    }
  }

  target << YAML::EndMap;
}

void TechniquePropertySet::load(const YAML::Node& source,
                                const fs::path& resourcesRootFolder)
{
  if(!source.IsDefined() || !source.IsMap()) return;

  for(YAML::const_iterator iNode = source.begin();
      iNode != source.end();
      iNode++)
  {
    std::string propertyName = iNode->first.as<std::string>("");
    if (propertyName.empty()) continue;

    //  Если настройки ещё нет, то добавим её
    _addProperty(propertyName);

    //  Найдем и загрузим настройку
    PropertiesMap::iterator iProperty = _properties.find(propertyName);
    MT_ASSERT(iProperty != _properties.end());
    iProperty->second->load(iNode->second, resourcesRootFolder);
  }
}
