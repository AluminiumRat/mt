#include <fstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

#include <gui/GUIWindow.h>
#include <gui/ImGuiRAII.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/modalDialogs.h>
#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <resourceManagement/TextureManager.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
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
  _sunAngleSize(0.01f),
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

void EnvironmentScene::save(const fs::path& fileName)
{
  fs::path projectFolder = fileName.parent_path();

  // Создаем YAML разметку
  YAML::Emitter out;

  out << YAML::BeginMap;

  out << YAML::Key << "sunAzimuth";
  out << YAML::Value << _sunAzimuth;

  out << YAML::Key << "sunAltitude";
  out << YAML::Value << _sunAltitude;

  out << YAML::Key << "sunAngleSize";
  out << YAML::Value << _sunAngleSize;

  out << YAML::Key << "directLightIrradiance";
  out << YAML::Value << _directLightIrradiance;

  out << YAML::Key << "directLightColor";
  out << YAML::Value;
  out << YAML::Flow << YAML::BeginSeq;
  out << _directLightColor.r << _directLightColor.g << _directLightColor.b;
  out << YAML::EndSeq;

  out << YAML::Key << "irradianceMap";
  out << YAML::Value << mt::pathToUtf8(mt::makeStoredPath(_irradianceMapFile,
                                                          projectFolder));

  out << YAML::Key << "specularMap";
  out << YAML::Value << mt::pathToUtf8(mt::makeStoredPath(_specularMapFile,
                                                          projectFolder));

  out << YAML::EndMap;

  // Сохраняем в файл
  std::ofstream fileStream(fileName, std::ios::binary);
  if(!fileStream.is_open())
  {
    throw std::runtime_error(std::string("Unable to open file ") + pathToUtf8(fileName));
  }
  fileStream.write(out.c_str(), out.size());
}

void EnvironmentScene::load(const fs::path& fileName)
{
  fs::path projectFolder = fileName.parent_path();

  std::vector<char> data = ContentLoader::getLoader().loadData(fileName);
  data.push_back(0);

  YAML::Node rootNode = YAML::Load(data.data());
  if(!rootNode.IsMap()) throw std::runtime_error(std::string("wrong file format: ") + pathToUtf8(fileName));

  {
    std::string irradianceMap = rootNode["irradianceMap"].as<std::string>("");
    if(!irradianceMap.empty())
    {
      fs::path irradianceMapFile = utf8ToPath(irradianceMap);
      irradianceMapFile = restoreAbsolutePath(irradianceMapFile, projectFolder);
      setIrradianceMap(irradianceMapFile);
    }
  }

  {
    std::string specularMap = rootNode["specularMap"].as<std::string>("");
    if(!specularMap.empty())
    {
      fs::path specularMapFile = utf8ToPath(specularMap);
      specularMapFile = restoreAbsolutePath(specularMapFile, projectFolder);
      setSpecularMap(specularMapFile);
    }
  }

  _sunAzimuth = rootNode["sunAzimuth"].as<float>(0.0f);
  _sunAltitude = rootNode["sunAltitude"].as<float>(pi / 4.0f);
  _sunAngleSize = rootNode["sunAngleSize"].as<float>(0.01f);
  _directLightIrradiance = rootNode["directLightIrradiance"].as<float>(3.0f);
  _directLightColor.x = rootNode["directLightColor"][0].as<float>(1.0f);
  _directLightColor.y = rootNode["directLightColor"][1].as<float>(1.0f);
  _directLightColor.z = rootNode["directLightColor"][2].as<float>(1.0f);
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
  ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGuiWindow window("Environment");
  if (window.visible())
  {
    {
      ImGuiPropertyGrid grid("EnvironmentGrid");
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

      grid.addRow("Sun size");
      float sunSize = _sunAngleSize * 360.0f / (2.0f * pi);
      if(ImGui::SliderFloat("##sunsize", &sunSize, 0, 10))
      {
        _sunAngleSize = sunSize / 360.0f * 2.0f * pi;
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
