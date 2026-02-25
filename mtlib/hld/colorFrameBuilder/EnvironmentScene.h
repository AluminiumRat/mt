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

    //  Расположение солнца на небе
    inline float sunAzimuth() const noexcept;
    inline void setSunAzimuth(float newValue) noexcept;
    inline float sunAltitude() const noexcept;
    inline void setSunAltitude(float newValue) noexcept;

    //  Освещенность от прямых лучей солнца на плоскости, перпендикулярной
    //    sunDirection
    //  Нормированноя освещенность равная 1 дает на чисто ламбертовом отражении
    //    нормированную яркость 1/pi
    inline float directLightIrradiance() const noexcept;
    inline void setDirectLightIrradiance(float newValue) noexcept;

    //  Цвет прямого света
    inline const glm::vec3& directLightColor() const noexcept;
    inline void setDirectLightColor(const glm::vec3& newValue) noexcept;

    //  Данные, которые должны быть отправлены в юниформ буфер
    inline UniformBufferData uniformData() const noexcept;

    //  Загрузить карты освещения. Управление вернется немедленно, загрузка
    //  будет проводиться через асинхронную таску
    void setIrradianceMap(const std::filesystem::path& irradianceMap);
    void setSpecularMap(const std::filesystem::path& specularMap);

    //  Префильтрованная карта освещенности для IBL
    inline const ImageView& irradianceMap() const noexcept;
    //  Префильтрованная specular карта для IBL
    inline const ImageView& specularMap() const noexcept;

    //  Добавить окно с настройками в текущий контекст ImGui
    void makeGui();

  private:
    //  Создать дефолтные(пустые) текстуры для IBL
    void _crateDefaultIBLMaps();

  private:
    Device& _device;
    TextureManager& _textureManager;

    float _sunAzimuth;
    float _sunAltitude;
    float _directLightIrradiance;
    glm::vec3 _directLightColor;

    ConstRef<TechniqueResource> _irradianceMapResource;
    ConstRef<ImageView> _defaultIrradianceMap;
    std::filesystem::path _irradianceMapFile;

    ConstRef<TechniqueResource> _specularMapResource;
    ConstRef<ImageView> _defaultSpecularMap;
    std::filesystem::path _specularMapFile;
  };

  inline float EnvironmentScene::sunAzimuth() const noexcept
  {
    return _sunAzimuth;
  }

  inline void EnvironmentScene::setSunAzimuth(float newValue) noexcept
  {
    _sunAzimuth = newValue;
  }

  inline float EnvironmentScene::sunAltitude() const noexcept
  {
    return _sunAltitude;
  }
  
  inline void EnvironmentScene::setSunAltitude(float newValue) noexcept
  {
    _sunAltitude = newValue;
  }

  inline float EnvironmentScene::directLightIrradiance() const noexcept
  {
    return _directLightIrradiance;
  }

  inline void EnvironmentScene::setDirectLightIrradiance(
                                                        float newValue) noexcept
  {
    _directLightIrradiance = newValue;
  }

  inline const glm::vec3& EnvironmentScene::directLightColor() const noexcept
  {
    return _directLightColor;
  }
  
  inline void EnvironmentScene::setDirectLightColor(
                                            const glm::vec3& newValue) noexcept
  {
    _directLightColor = newValue;
  }

  inline EnvironmentScene::UniformBufferData
                                  EnvironmentScene::uniformData() const noexcept
  {
    UniformBufferData bufferData{};
    bufferData.toSunDirection = glm::vec3(
                                          cos(_sunAzimuth) * cos(_sunAltitude),
                                          sin(_sunAzimuth) * cos(_sunAltitude),
                                          sin(_sunAltitude));
    bufferData.fromSunDirection = -bufferData.toSunDirection;
    bufferData.directLightIrradiance =
                                    _directLightIrradiance * _directLightColor;
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