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
    static constexpr size_t uniformBufferSize = sizeof(UniformBufferData);

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

    //  Обновить данные GPU ресурсов.
    virtual void update();

    //  Юниформ буфер с данными UniformBufferData
    //  Буфером владеет device.primaryQueue
    inline const DataBuffer& uniformBuffer() const noexcept;

  private:
    Device& _device;

    glm::vec3 _sunDirection;
    glm::vec3 _directLightIrradiance;

    Ref<DataBuffer> _uniformBuffer;
    bool _needUpdateUniformBuffer;
  };

  inline const glm::vec3& GlobalLight::sunDirection() noexcept
  {
    return _sunDirection;
  }

  inline void GlobalLight::setSunDirection(const glm::vec3& newValue) noexcept
  {
    if(_sunDirection == newValue) return;
    _sunDirection = glm::normalize(newValue);
    _needUpdateUniformBuffer = true;
  }

  inline const glm::vec3& GlobalLight::directLightIrradiance() noexcept
  {
    return _directLightIrradiance;
  }

  inline void GlobalLight::setDirectLightIrradiance(
                                            const glm::vec3& newValue) noexcept
  {
    if(_directLightIrradiance == newValue) return;
    _directLightIrradiance = newValue;
    _needUpdateUniformBuffer = true;
  }

  inline const DataBuffer& GlobalLight::uniformBuffer() const noexcept
  {
    MT_ASSERT(_uniformBuffer != nullptr);
    return *_uniformBuffer;
  }
};