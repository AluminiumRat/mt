#include <fstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

#include <gui/BaseWindow.h>
#include <gui/GUILib.h>
#include <util/Log.h>
#include <vkr/VKRLib.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vkr/Win32WindowSurface.h>

namespace fs = std::filesystem;

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

void GUILib::loadConfiguration(const fs::path& file) noexcept
{
  try
  {
    std::ifstream fileStream(file, std::ios::binary);
    if(!fileStream) throw std::runtime_error(std::string("config file not found: ") + (const char*)file.u8string().c_str());
    YAML::Node configNode = YAML::Load(fileStream);
    if(!configNode.IsDefined() || !configNode.IsMap()) throw std::runtime_error(std::string("wrong config file format: ") + (const char*)file.u8string().c_str());

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
        configuration.imguiConfig =
                              propertiesNode["imguiConfig"].as<std::string>("");

        _configurationMap[windowName] = configuration;
      }
    }
  }
  catch(std::exception& error)
  {
    Log::warning() << "GUILib::loadConfiguration: " << error.what();
  }
}

void GUILib::saveConfiguration(const fs::path& file) const noexcept
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
      out << YAML::Key << "imguiConfig";
      out << YAML::Value << windowProps.imguiConfig;

      out << YAML::EndMap;
    }

    out << YAML::EndMap;

    // А теперь сохраняем это всё в файл
    std::ofstream fileStream(file, std::ios::binary);
    if(!fileStream.is_open())
    {
      throw std::runtime_error(std::string("Unable to open file ") + (const char*)file.u8string().c_str());
    }
    fileStream.write(out.c_str(), out.size());
  }
  catch (std::exception& error)
  {
    Log::warning() << "GUILib::loadConfiguration: " << error.what();
  }
}

std::unique_ptr<Device> GUILib::createDevice(
                            const VkPhysicalDeviceFeatures& requiredFeatures,
                            const VkPhysicalDeviceVulkan12Features&
                                                      requiredFeaturesVulkan12,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* testWindow = glfwCreateWindow(1, 1, "Test window", nullptr, nullptr);

  std::unique_ptr<Device> device;
  try
  {
    Win32WindowSurface testSurface(glfwGetWin32Window(testWindow));
    device = VKRLib::instance().createDevice( requiredFeatures,
                                              requiredFeaturesVulkan12,
                                              requiredExtensions,
                                              configuration,
                                              &testSurface);
  }
  catch(...)
  {
    glfwDestroyWindow(testWindow);
    throw;
  }

  glfwDestroyWindow(testWindow);
  return device;
}