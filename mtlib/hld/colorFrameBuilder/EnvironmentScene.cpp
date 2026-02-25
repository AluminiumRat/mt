#include <stdexcept>

#include <gui/GUIWindow.h>
#include <gui/ImGuiRAII.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/modalDialogs.h>
#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <resourceManagement/TextureManager.h>
#include <util/pi.h>
#include <vkr/Device.h>

namespace fs = std::filesystem;

using namespace mt;

EnvironmentScene::EnvironmentScene( Device& device,
                                    TextureManager& textureManager) :
  _device(device),
  _textureManager(textureManager),
  _sunAzimuth(0.0f),
  _sunAltitude(pi / 4.0f),
  _directLightIrradiance(3),
  _directLightColor(1.0f)
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

void EnvironmentScene::setIrradianceMap(const fs::path& irradianceMap)
{
  try
  {
    _irradianceMapResource =
                        _textureManager.scheduleLoading(irradianceMap,
                                                        *_device.graphicQueue(),
                                                        false);
    _irradianceMapFile = irradianceMap;
  }
  catch(...)
  {
    _irradianceMapResource = nullptr;
    _irradianceMapFile.clear();
    throw;
  }
}

void EnvironmentScene::setSpecularMap(const fs::path& specularMap)
{
  try
  {
    _specularMapResource =
                        _textureManager.scheduleLoading(specularMap,
                                                        *_device.graphicQueue(),
                                                        false);
    _specularMapFile = specularMap;
  }
  catch(...)
  {
    _specularMapResource = nullptr;
    _specularMapFile.clear();
    throw;
  }
}

void EnvironmentScene::makeGui()
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGuiWindow window("Environment");
  if (window.visible())
  {
    {
      ImGuiPropertyGrid grid("ColorGrading");
      grid.addRow("Sun azimuth");
      float azimuth = _sunAzimuth * 360.0f / (2.0f * pi);
      if(ImGui::SliderFloat("##azimuth",&azimuth, -360, 360))
      {
        _sunAzimuth = azimuth / 360.0f * 2.0f * pi;
      }

      grid.addRow("Sun altitude");
      float altitude = _sunAltitude * 360.0f / (2.0f * pi);
      if(ImGui::SliderFloat("##altitude", &altitude, -90, 90))
      {
        _sunAltitude = altitude / 360.0f * 2.0f * pi;
      }

      grid.addRow("Sun irradiance");
      ImGui::InputFloat("##irradiance", &_directLightIrradiance);

      grid.addRow("Sun color");
      colorPicker("##sunColor", _directLightColor);

      grid.addRow("Irradiance map");
      if(fileSelectionLine("##irradianceMap", _irradianceMapFile))
      {
        fs::path file =
              openFileDialog( GUIWindow::currentWindow(),
                              FileFilters{{ .expression = "*.dds",
                                            .description = "DDS image(*.dds)"}},
                              "");
        if(!file.empty()) setIrradianceMap(file);
      }

      grid.addRow("Specular map");
      if(fileSelectionLine("##specularMap", _specularMapFile))
      {
        fs::path file = openFileDialog( GUIWindow::currentWindow(),
                                        FileFilters{
                                          { .expression = "*.dds",
                                            .description = "DDS image(*.dds)"}},
                                        "");
        if(!file.empty()) setSpecularMap(file);
      }
    }

    window.end();
  }
}
