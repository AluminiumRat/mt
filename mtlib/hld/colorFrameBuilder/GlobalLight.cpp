#include <util/pi.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <vkr/Device.h>

using namespace mt;

GlobalLight::GlobalLight(Device& device) :
  _device(device),
  _sunDirection(glm::normalize(glm::vec3(-1, 0, -1))),
  _directLightIrradiance(pi),
  _uniformBuffer(new DataBuffer(device,
                                uniformBufferSize,
                                DataBuffer::UNIFORM_BUFFER,
                                "Global light")),
  _needUpdateUniformBuffer(true)
{
}

void GlobalLight::update()
{
  if(_needUpdateUniformBuffer)
  {
    UniformBufferData bufferData{};
    bufferData.fromSunDirection = -_sunDirection;
    bufferData.toSunDirection = _sunDirection;
    bufferData.directLightIrradiance = _directLightIrradiance;

    _device.primaryQueue().uploadToBuffer(*_uniformBuffer,
                                          0,
                                          uniformBufferSize,
                                          &bufferData);
    _needUpdateUniformBuffer = false;
  }
}