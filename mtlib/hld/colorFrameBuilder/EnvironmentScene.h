#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <technique/TechniqueResource.h>
#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Device;
  class TextureManager;

  class EnvironmentScene
  {
  public:
    struct UniformBufferData
    {
      alignas(16) glm::vec3 fromSunDirection;
      alignas(16) glm::vec3 toSunDirection;
      alignas(16) glm::vec3 directLightIrradiance;
      //  Коэффициент для перевода рафнеса материала в номер лода спекулар мапы
      alignas(4) float roughnessToLod;
    };

  public:
    EnvironmentScene(Device& device, TextureManager& textureManager);
    EnvironmentScene(const EnvironmentScene&) = delete;
    EnvironmentScene& operator = (const EnvironmentScene&) = delete;
    virtual ~EnvironmentScene() noexcept = default;

    //  Направление света от солнца (указывает в противоположном от солнца
    //    направлении)
    inline const glm::vec3& sunDirection() noexcept;
    inline void setSunDirection(const glm::vec3& newValue) noexcept;

    //  Освещенность от прямых лучей солнца на плоскости, перпендикулярной
    //    sunDirection
    //  Нормированноя освещенность равная 1 дает на чисто ламбертовом отражении
    //    нормированную яркость 1/pi
    inline const glm::vec3& directLightIrradiance() noexcept;
    inline void setDirectLightIrradiance(const glm::vec3& newValue) noexcept;

    //  Данные, которые должны быть отправлены в юниформ буфер
    inline UniformBufferData uniformData() const noexcept;

    //  Загрузить карты освещения. Управление вернется немедленно, загрузка
    //  будет проводиться через асинхронную таску
    void setIBLMaps(const std::filesystem::path& irradianceMap,
                    const std::filesystem::path& specularMap);

    //  Префильтрованная карта освещенности для IBL
    inline const ImageView& irradianceMap() const noexcept;
    //  Префильтрованная specular карта для IBL
    inline const ImageView& specularMap() const noexcept;

  private:
    //  Создать дефолтные(пустые) текстуры для IBL
    void _crateDefaultIBLMaps();

  private:
    Device& _device;
    TextureManager& _textureManager;

    glm::vec3 _sunDirection;
    glm::vec3 _directLightIrradiance;

    ConstRef<TechniqueResource> _irradianceMapResource;
    ConstRef<ImageView> _defaultIrradianceMap;

    ConstRef<TechniqueResource> _specularMapResource;
    ConstRef<ImageView> _defaultSpecularMap;
  };

  inline const glm::vec3& EnvironmentScene::sunDirection() noexcept
  {
    return _sunDirection;
  }

  inline void EnvironmentScene::setSunDirection(const glm::vec3& newValue) noexcept
  {
    _sunDirection = glm::normalize(newValue);
  }

  inline const glm::vec3& EnvironmentScene::directLightIrradiance() noexcept
  {
    return _directLightIrradiance;
  }

  inline void EnvironmentScene::setDirectLightIrradiance(
                                            const glm::vec3& newValue) noexcept
  {
    _directLightIrradiance = newValue;
  }

  inline EnvironmentScene::UniformBufferData
                                  EnvironmentScene::uniformData() const noexcept
  {
    UniformBufferData bufferData{};
    bufferData.fromSunDirection = _sunDirection;
    bufferData.toSunDirection = -_sunDirection;
    bufferData.directLightIrradiance = _directLightIrradiance;
    bufferData.roughnessToLod = float(specularMap().image().mipmapCount() - 1);
    return bufferData;
  }

  inline const ImageView& EnvironmentScene::irradianceMap() const noexcept
  {
    if(_irradianceMapResource->image() != nullptr)
    {
      return *_irradianceMapResource->image();
    }
    return *_defaultIrradianceMap;
  }

  inline const ImageView& EnvironmentScene::specularMap() const noexcept
  {
    if(_specularMapResource->image() != nullptr)
    {
      return *_specularMapResource->image();
    }
    return *_defaultSpecularMap;
  }
};