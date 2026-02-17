#pragma once

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Device;

  class EnvironmentScene
  {
  public:
    struct UniformBufferData
    {
      alignas(16) glm::vec3 fromSunDirection;
      alignas(16) glm::vec3 toSunDirection;
      alignas(16) glm::vec3 directLightIrradiance;
    };

  public:
    explicit EnvironmentScene(Device& device);
    EnvironmentScene(const EnvironmentScene&) = delete;
    EnvironmentScene& operator = (const EnvironmentScene&) = delete;
    virtual ~EnvironmentScene() noexcept = default;

    //  Направление света от солнца (указывает в противоположном от солнца
    //    направлении)
    inline const glm::vec3& sunDirection() noexcept;
    inline void setSunDirection(const glm::vec3& newValue) noexcept;

    //  Нормированное значение освещенности от прямых лучей солнца по
    //    компонентам rgb на плоскости, перпендикулярной sunDirection
    //  Нормированноя освещенность равная 1 дает на чисто ламбертовом отражении
    //    нормированную яркость 1/pi
    inline const glm::vec3& directLightIrradiance() noexcept;
    inline void setDirectLightIrradiance(const glm::vec3& newValue) noexcept;

    //  Данные, которые должны быть отправлены в юниформ буфер
    inline UniformBufferData uniformData() const noexcept;

  private:
    Device& _device;

    glm::vec3 _sunDirection;
    glm::vec3 _directLightIrradiance;
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
    return bufferData;
  }
};