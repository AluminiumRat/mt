#include <fstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

#include <gui/BaseWindow.h>
#include <gui/GUILib.h>
#include <util/Log.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace mt;

GUILib* GUILib::_instance = nullptr;

GUILib::GUILib()
{
  MT_ASSERT(_instance == nullptr);
  if(glfwInit() == GLFW_FALSE) throw std::runtime_error("GUILib: unable to init GLFW");
  _instance = this;
}

GUILib::~GUILib() noexcept
{
  MT_ASSERT(_instance != nullptr);
  MT_ASSERT(_windows.empty());

  glfwTerminate();

  _instance = nullptr;
}

void GUILib::updateWindows()
{
  glfwPollEvents();
  for(BaseWindow* window : _windows) window->update();
}

void GUILib::drawWindows()
{
  for (BaseWindow* window : _windows) window->draw();
}

bool GUILib::shouldBeClosed() const noexcept
{
  for(const BaseWindow* window : _windows)
  {
    if(!window->isClosed()) return false;
  }
  return true;
}

void GUILib::loadConfiguration(const char* filename) noexcept
{
  try
  {
    YAML::Node configNode = YAML::LoadFile(filename);
    if(!configNode.IsMap()) throw std::runtime_error(std::string("GUILib: wrong config file format: ") + filename);

    _configurationMap.clear();

    for(YAML::const_iterator iWindowConfig = configNode.begin();
        iWindowConfig != configNode.end();
        iWindowConfig++)
    {
      std::string windowName = iWindowConfig->first.as<std::string>("");
      if(!windowName.empty())
      {
        YAML::Node propertiesNode = iWindowConfig->second;

        WindowConfiguration configuration{};
        configuration.width =
                          propertiesNode["width"].as<int>(configuration.width);
        configuration.height =
                        propertiesNode["height"].as<int>(configuration.height);
        configuration.xPos = propertiesNode["xPos"].as<int>(configuration.xPos);
        configuration.yPos = propertiesNode["yPos"].as<int>(configuration.yPos);
        configuration.isMaximized =
              propertiesNode["isMaximized"].as<bool>(configuration.isMaximized);
        configuration.isMinimized =
              propertiesNode["isMinimized"].as<bool>(configuration.isMinimized);

        _configurationMap[windowName] = configuration;
      }
    }
  }
  catch(std::exception& error)
  {
    Log::warning() << "GUILib::loadConfiguration: " << error.what();
  }
}

void GUILib::saveConfiguration(const char* filename) const noexcept
{
  try
  {
    // Обходим ещё открытые окна и получаем от них конфигурации
    for (const BaseWindow* window : _windows) window->saveConfiguration();

    // Создаем YAML разметку для конфигурации
    YAML::Emitter out;

    out << YAML::BeginMap;

    for(ComfigurationMap::const_iterator iConfig = _configurationMap.begin();
        iConfig != _configurationMap.end();
        iConfig++)
    {
      out << YAML::Key << iConfig->first;

      out << YAML::Value;
      out << YAML::BeginMap;

      const WindowConfiguration& windowProps = iConfig->second;
      out << YAML::Key << "width" << YAML::Value << windowProps.width;
      out << YAML::Key << "height" << YAML::Value << windowProps.height;
      out << YAML::Key << "xPos" << YAML::Value << windowProps.xPos;
      out << YAML::Key << "yPos" << YAML::Value << windowProps.yPos;
      out << YAML::Key << "isMaximized";
      out << YAML::Value << windowProps.isMaximized;
      out << YAML::Key << "isMinimized";
      out << YAML::Value << windowProps.isMinimized;

      out << YAML::EndMap;
    }

    out << YAML::EndMap;

    // А теперь сохраняем это всё в файл
    std::ofstream file(filename, std::ios::binary);
    if(!file.is_open())
    {
      throw std::runtime_error(std::string("Unable to open file ") + filename);
    }
    file.write(out.c_str(), out.size());
  }
  catch (std::exception& error)
  {
    Log::warning() << "GUILib::loadConfiguration: " << error.what();
  }
}
