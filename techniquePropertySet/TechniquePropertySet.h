#pragma once

#include <map>
#include <memory>

#include <techniquePropertySet/TechniqueProperty.h>
#include <techniquePropertySet/TechniquePropertySetCommon.h>

namespace mt
{
  class Technique;

  //  Набор настроек для техники. Позволяет сохранять/загружать значения
  //  униформов и бинды ресурсов для техник. Также используется для настройки
  //  техники через GUI
  class TechniquePropertySet
  {
  public:
    TechniquePropertySet( mt::Technique& technique,
                          TextureManager& textureManager,
                          BufferResourceManager& bufferManager,
                          CommandQueueTransfer& resourceOwnerQueue);
    TechniquePropertySet(const TechniquePropertySet&) = delete;
    TechniquePropertySet& operator = (const TechniquePropertySet&) = delete;
    virtual ~TechniquePropertySet() = default;

    inline mt::Technique& technique() const noexcept;

    //  resourcesRootFolder - корневая папка для ресурсов. Если ресурс находится
    //  в этой папке(или подпапке), то путь до него будет сохранен как
    //  относительный, иначе - как абсолютный
    void save(YAML::Emitter& target,
              const std::filesystem::path& resourcesRootFolder) const;

    //  resourcesRootFolder - корневая папка для ресурсов. Если ресурс сохранен
    //  через относительный путь, то абсолютный путь до ресурса будет
    //  восстановлен с помощью resourcesRootFolder
    void load(const YAML::Node& source,
              const std::filesystem::path& resourcesRootFolder);

    //  Обновить информацию о технике из TechniqueConfiguration
    //  ВНИМАНИЕ! Этот метод не читает значения юниформов и пути ресурсов из
    //    техники. Он обновляет типы и состав настроек
    void updateFromTechnique();

    //  Константный обход настроек. functor должен поддерживать сигнатуру
    //  void(const TechniqueProperty&)
    template<typename EnumerateFunctor>
    inline void enumerateProperties(EnumerateFunctor functor) const;

    //  Неонстантный обход настроек. functor должен поддерживать сигнатуру
    //  void(TechniqueProperty&)
    template<typename EnumerateFunctor>
    inline void enumerateProperties(EnumerateFunctor functor);

  private:
    void _addProperty(const std::string& name);

  private:
    mt::Technique& _technique;
    size_t _lastConfigurationRevision;

    TechniquePropertySetCommon _commonData;

    using PropertiesMap =
                      std::map<std::string, std::unique_ptr<TechniqueProperty>>;
    PropertiesMap _properties;
  };

  inline mt::Technique& TechniquePropertySet::technique() const noexcept
  {
    return _technique;
  }

  template<typename EnumerateFunctor>
  inline void TechniquePropertySet::enumerateProperties(
                                                EnumerateFunctor functor) const
  {
    for(PropertiesMap::const_iterator iProperty = _properties.begin();
        iProperty != _properties.end();
        iProperty++)
    {
      const TechniqueProperty& property = *iProperty->second;
      functor(property);
    }
  }

  template<typename EnumerateFunctor>
  inline void TechniquePropertySet::enumerateProperties(
                                                      EnumerateFunctor functor)
  {
    for(PropertiesMap::iterator iProperty = _properties.begin();
        iProperty != _properties.end();
        iProperty++)
    {
      TechniqueProperty& property = *iProperty->second;
      functor(property);
    }
  }
}