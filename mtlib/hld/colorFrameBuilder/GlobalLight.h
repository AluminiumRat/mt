#pragma once

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Device;

  class GlobalLight
  {
  public:
    struct UniformBufferData
    {
      alignas(16) glm::vec3 fromSunDirection;
      alignas(16) glm::vec3 toSunDirection;
      alignas(16) glm::vec3 directLightIrradiance;
    };

  public:
    explicit GlobalLight(Device& device);
    GlobalLight(const GlobalLight&) = delete;
    GlobalLight& operator = (const GlobalLight&) = delete;
    virtual ~GlobalLight() noexcept = default;

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

  inline const glm::vec3& GlobalLight::sunDirection() noexcept
  {
    return _sunDirection;
  }

  inline void GlobalLight::setSunDirection(const glm::vec3& newValue) noexcept
  {
    _sunDirection = glm::normalize(newValue);
  }

  inline const glm::vec3& GlobalLight::directLightIrradiance() noexcept
  {
    return _directLightIrradiance;
  }

  inline void GlobalLight::setDirectLightIrradiance(
                                            const glm::vec3& newValue) noexcept
  {
    _directLightIrradiance = newValue;
  }

  inline GlobalLight::UniformBufferData
                                      GlobalLight::uniformData() const noexcept
  {
    UniformBufferData bufferData{};
    bufferData.fromSunDirection = _sunDirection;
    bufferData.toSunDirection = -_sunDirection;
    bufferData.directLightIrradiance = _directLightIrradiance;
    return bufferData;
  }
};