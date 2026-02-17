#include <stdexcept>

#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <resourceManagement/TextureManager.h>
#include <util/pi.h>
#include <vkr/Device.h>

using namespace mt;

EnvironmentScene::EnvironmentScene( Device& device,
                                    TextureManager& textureManager) :
  _device(device),
  _textureManager(textureManager),
  _sunDirection(glm::normalize(glm::vec3(-1, 0, -1))),
  _directLightIrradiance(pi)
{
  _crateDefaultIBLMaps();
}

void EnvironmentScene::_crateDefaultIBLMaps()
{
  _irradianceMapResource =
                    _textureManager.loadImmediately("util/defaultIbl_CUBEMAP.dds",
                                                    *_device.graphicQueue(),
                                                    false);
  if(_irradianceMapResource->image() == nullptr) throw std::runtime_error("EnvironmentScene: unable to load 'util/defaultIbl_CUBEMAP.dds'");
  _defaultIrradianceMap = _irradianceMapResource->image();

  _specularMapResource = _irradianceMapResource;
  _defaultSpecularMap = _specularMapResource->image();
}

void EnvironmentScene::setIBLMaps(const std::filesystem::path& irradianceMap,
                                  const std::filesystem::path& specularMap)
{
    _irradianceMapResource =
                        _textureManager.scheduleLoading(irradianceMap,
                                                        *_device.graphicQueue(),
                                                        false);
    _specularMapResource =
                        _textureManager.scheduleLoading(specularMap,
                                                        *_device.graphicQueue(),
                                                        false);
}
