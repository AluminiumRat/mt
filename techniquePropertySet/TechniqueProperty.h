#pragma once

#include <memory>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>

#include <technique/ResourceBinding.h>
#include <technique/Technique.h>
#include <technique/UniformVariable.h>
#include <techniquePropertySet/TechniquePropertySetCommon.h>

namespace YAML
{
  class Emitter;
  class Node;
};

namespace mt
{
  //  Отдельная настройка для техники. Работает в составе PropertySet-а
  //  Унифицирует доступ к юниформам и ресурсам, позволяет сохранять и загружать
  //  настройки техники
  class TechniqueProperty
  {
  public:
    enum SamplerMode
    {
      DEFAULT_SAMPLER_MODE,
      CUSTOM_SAMPLER_MODE
    };
    struct SamplerValue
    {
      SamplerMode mode = DEFAULT_SAMPLER_MODE;
      SamplerDescription description;

      bool operator == (const SamplerValue&) const noexcept = default;
    };

  public:
    //  name - имя юниформа или ресурса, по которому к нему идет обращение в
    //    шейдере и технике. Для юниформов это имяБуфера.имяПеременной, для
    //    текстур, буферов и сэмплеров - просто имя ресурса в шейдере
    TechniqueProperty(Technique& technique,
                      const std::string& name,
                      const TechniquePropertySetCommon& commonData);
    TechniqueProperty(const TechniqueProperty&) = delete;
    TechniqueProperty& operator = (const TechniqueProperty&) = delete;
    ~TechniqueProperty() noexcept = default;

    inline const std::string& name() const noexcept;
    //  Для юниформы shortName -  название без имени юниформа буфера. Для
    //    ресурсов name и shortName совпадают - имя соответствующего ресурса из
    //    шейдера.
    //  shortName корректен, только если проперти активно (метод isActive)
    inline const std::string& shortName() const noexcept;

    //  Если true, значит эта проперти соответствует рабочей униформе или
    //  ресурсу в технике. Если false, значит техника не использует униформу
    //  или ресурс с таким именем
    inline bool isActive() const noexcept;

    //  Проперти не поддерживает тип ресурса или юниформы, к которой
    //  она привязана
    inline bool unsupportedType() const noexcept;

    //  Битсет из TechniqueConfiguration::GUIHints
    inline uint32_t guiHints() const noexcept;

    inline bool isUniform() const noexcept;
    inline TechniqueConfiguration::BaseScalarType scalarType() const noexcept;
    //  Если это юниформа, то возвращает её размерность
    //  1 для float и int, 2 для vec2 и ivec2 и т.д
    inline uint32_t vectorSize() const noexcept;
    inline const glm::ivec4& intValue() const noexcept;
    inline const glm::vec4& floatValue() const noexcept;
    void setUniformValue(const glm::ivec4& newValue);
    void setUniformValue(const glm::vec4& newValue);

    inline bool isResource() const noexcept;
    inline VkDescriptorType resourceType() const noexcept;
    inline const std::filesystem::path& resourcePath() const noexcept;
    void setResourcePath(const std::filesystem::path& newValue);

    inline bool isSampler() const noexcept;
    //  Если isSample возвращает true, то всегда вернет ненулевой указатель,
    //    иначе может вернуть nullptr
    inline const SamplerValue* samplerValue() const noexcept;
    void setSamplerValue(const SamplerValue& newValue);

    //  Обновить информацию о проперти из TechniqueConfiguration
    //  ВНИМАНИЕ! Этот метод не читает значения юниформов и пути ресурсов из
    //    техники. Он обновляет тип проперти и флаг isActive
    void updateFromTechnique();

    //  resourcesRootFolder - корневая папка для ресурсов. Если ресурс находится
    //  в этой папке(или подпапке), то путь до него будет сохранен как
    //  относительный, иначе - как абсолютный
    void save(YAML::Emitter& target,
              const std::filesystem::path& resourcesRootFolder) const;

    //  Загрузка проперти из YAML ноды.
    //  ВНИАНИЕ! Не считывает имя проперти из ноды,
    //    предполагая, что fullName и shortName корректно указаны при
    //    конструировании
    //  resourcesRootFolder - корневая папка для ресурсов. Если ресурс сохранен
    //    через относительный путь, то абсолютный путь до ресурса будет
    //    восстановлен с помощью resourcesRootFolder
    void load(const YAML::Node& source,
              const std::filesystem::path& resourcesRootFolder);

  private:
    const TechniqueConfiguration::UniformVariable*
                                        _getUniformDescription() const noexcept;
    const TechniqueConfiguration::Resource*
                                      _getResourceDescription() const noexcept;
    void _updateUniformValue();
    void _updateResource();
    void _updateSampler();
    SamplerValue& _getOrCreateSamplerValue();

    void _saveUniform(YAML::Emitter& target) const;
    void _saveResource( YAML::Emitter& target,
                        const std::filesystem::path& resourcesRootFolder) const;
    void _saveSampler(YAML::Emitter& target) const;
    bool _tryReadUniform(const YAML::Node& source);
    bool _tryReadResource(const YAML::Node& source,
                          const std::filesystem::path& resourcesRootFolder);
    bool _tryReadSampler(const YAML::Node& source);

  private:
    Technique& _technique;

    std::string _name;
    std::string _shortName;

    const TechniquePropertySetCommon& _commonData;

    bool _isActive;
    bool _unsupportedType;

    uint32_t _guiHints;

    UniformVariable* _uniform;
    TechniqueConfiguration::BaseScalarType _scalarType;
    uint32_t _vectorSize;
    glm::ivec4 _intValue;
    glm::vec4 _floatValue;

    ResourceBinding* _resourceBinding;
    VkDescriptorType _resourceType;
    std::filesystem::path _resourcePath;

    std::unique_ptr<SamplerValue> _samplerValue;
  };

  inline const std::string& TechniqueProperty::name() const noexcept
  {
    return _name;
  }

  inline const std::string& TechniqueProperty::shortName() const noexcept
  {
    return _shortName;
  }

  inline bool TechniqueProperty::isActive() const noexcept
  {
    return _isActive;
  }

  inline bool TechniqueProperty::unsupportedType() const noexcept
  {
    return _unsupportedType;
  }

  inline uint32_t TechniqueProperty::guiHints() const noexcept
  {
    return _guiHints;
  }

  inline bool TechniqueProperty::isUniform() const noexcept
  {
    return isActive() && _uniform != nullptr;
  }

  inline TechniqueConfiguration::BaseScalarType
                                  TechniqueProperty::scalarType() const noexcept
  {
    return _scalarType;
  }

  inline uint32_t TechniqueProperty::vectorSize() const noexcept
  {
    return _vectorSize;
  }

  inline const glm::ivec4& TechniqueProperty::intValue() const noexcept
  {
    return _intValue;
  }

  inline const glm::vec4& TechniqueProperty::floatValue() const noexcept
  {
    return _floatValue;
  }

  inline bool TechniqueProperty::isResource() const noexcept
  {
    return isActive() && _resourceBinding != nullptr;
  }

  inline VkDescriptorType TechniqueProperty::resourceType() const noexcept
  {
    return _resourceType;
  }

  inline const std::filesystem::path&
                                TechniqueProperty::resourcePath() const noexcept
  {
    return _resourcePath;
  }

  inline bool TechniqueProperty::isSampler() const noexcept
  {
    return isResource() && _resourceType == VK_DESCRIPTOR_TYPE_SAMPLER;
  }

  inline const TechniqueProperty::SamplerValue*
                                TechniqueProperty::samplerValue() const noexcept
  {
    return _samplerValue.get();
  }
}