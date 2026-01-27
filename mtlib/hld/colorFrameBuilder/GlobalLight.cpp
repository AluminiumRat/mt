#include <util/pi.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <vkr/Device.h>

using namespace mt;

GlobalLight::GlobalLight(Device& device) :
  _device(device),
  _sunDirection(glm::normalize(glm::vec3(-1, 0, -1))),
  _directLightIrradiance(pi)
{
}