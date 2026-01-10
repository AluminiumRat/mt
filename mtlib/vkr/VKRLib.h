#pragma once

#include <cstdlib>
#include <memory>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <util/util.h>

namespace mt
{
  class WindowSurface;

  // Синглтон для библиотеки рендера через вулкан
  // Объект класса должен быть явно создан через конструктор и существовать
  // на протяжении всего времени работы с рендером.
  // Доступ к объекту можно производить через метод instance, но только после
  // явного создания объекта
  // Все static методы класса кроме instance могут быть вызваны без создания
  // экзэмпляра VKRLib. Они предназначены для получения предварительной
  // информации до вызова конструктор VKRLib
  class VKRLib
  {
  public:
    static constexpr uint32_t minimalVulkanAPIVersion = VK_API_VERSION_1_3;

  public:
    // Вспомогательна структура, чтобы передать верию приложения в конструктор
    struct AppVersion
    {
      uint32_t major = 1;
      uint32_t minor = 0;
      uint32_t patch = 0;
    };

  public:
    // vulkanAPIVersion не может быть ниже, чем minimalVulkanAPIVersion
    // enableValidation включает лэйер валидации и отправляет сообщения от него
    // в лог
    // enableDebug включает расширение VK_EXT_debug_utils, на котором работают
    // дебаг маркеры
    VKRLib( const char* applicationName,
            const AppVersion& applicationVersion,
            uint32_t vulkanAPIVersion = minimalVulkanAPIVersion,
            bool enableValidation = false,
            bool enableDebug = false);
    VKRLib(const VKRLib&) = delete;
    VKRLib& operator = (const VKRLib&) = delete;
    virtual ~VKRLib() noexcept;

    // Нельзя вызывать до создания VKRLib
    inline static VKRLib& instance() noexcept;

    inline VkInstance handle() const noexcept;

    inline uint32_t vulkanApiVersion() const noexcept;

    static bool isValidationSupported();
    inline bool isValidationEnabled() const noexcept;

    static bool isDebugSupported();
    inline bool isDebugEnabled() const noexcept;

    inline std::vector<PhysicalDevice*> devices() const;
    // Выбрать самое сильное физическое устройство.
    // Может вернуть nullptr, если не нашлось ни одного подходящего устройства
    // requiredExtensions - имена расширений, которые должны поддерживаться
    //    устройством
    // testSurface нужна для проверки совместимости устройства с оконной
    //    системой. Если передать nullptr, то проверка на совместимость не будет
    //    производиться
    PhysicalDevice* getBestPhysicalDevice(
                            VkPhysicalDeviceFeatures requiredFeatures,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration,
                            const WindowSurface* testSurface) const;

    // Создать логическое устройство из наиболее сильного физического
    // Может вернуть nullptr, если не нашлось ни одного подходящего устройства
    // requiredExtensions - имена расширений, которые должны поддерживаться
    //    устройством
    // Если testSurface не nullptr, то будет выбрано устройство, которое может
    //    выводить картинку на эту поверхность, а также будет создана очередь
    //    презентации. Если testSurface == nullptr, то созданное устройство не
    //    будет поддерживать вывод в оконную систему
    std::unique_ptr<Device> createDevice(
                            const VkPhysicalDeviceFeatures& requiredFeatures,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration,
                            const WindowSurface* testSurface) const;

    // Создать логическое устройство из конкретного физического
    // Всегда возвращает валидный указатель или выбрасывает исключение
    // Соответствие physicalDevice указанным требованиям на совести вызывающего
    // Если testSurface не nullptr, то будет создана очередь презентации.
    //    Если testSurface == nullptr, то созданное устройство не
    //    будет поддерживать вывод в оконную систему
    std::unique_ptr<Device> createDevice(
                              PhysicalDevice& physicalDevice,
                              VkPhysicalDeviceFeatures requiredFeatures,
                              const std::vector<std::string>& enabledExtensions,
                              QueuesConfiguration configuration,
                              const WindowSurface* testSurface) const;

    static std::vector<VkLayerProperties> availableLayers();
    inline static bool isLayerSupported(const char* layerName);

    static std::vector<VkExtensionProperties> availableExtensions();
    inline static bool isExtensionSupported(const char* extensionName);

  private:
    void _cleanup() noexcept;
    void _createVKInstance( const char* applicationName,
                            const AppVersion& applicationVersion);
    static void _populateDebugInfo(
                                VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void _setupDebugMessenger();
    void _receivePhysicalDevices();

    // Добавить в пользовательский список фич те фичи, которые требует движек
    void _extendRequiredFeatures(
                              VkPhysicalDeviceFeatures& requiredFeatures) const;

    // Добавить в пользовательский список расширений те, которые требует движек
    std::set<std::string> _extendRequiredExtensions(
                              const std::vector<std::string>& requiredExtensions,
                              bool needPresent) const;

    static bool _isDeviceSuitable(
                              const PhysicalDevice& device,
                              const VkPhysicalDeviceFeatures& requiredFeatures,
                              const std::set<std::string>& requiredExtensions,
                              QueuesConfiguration configuration,
                              const WindowSurface* testSurface);

  private:
    VkInstance _handle;
    VkDebugUtilsMessengerEXT _debugMessenger;
    bool _isValidationEnabled;
    bool _isDebugEnabled;

    uint32_t _vulkanApiVersion;

    using PhysicalDevices = std::vector<std::unique_ptr<PhysicalDevice>>;
    PhysicalDevices _devices;

    static VKRLib* _instance;
  };

  inline VKRLib& VKRLib::instance() noexcept
  {
    MT_ASSERT((_instance != nullptr) && "VKRLib::instance: VKRLib is not initialized.")
    return *_instance;
  }

  inline VkInstance VKRLib::handle() const noexcept
  {
    return _handle;
  }

  inline uint32_t VKRLib::vulkanApiVersion() const noexcept
  {
    return _vulkanApiVersion;
  }

  inline bool VKRLib::isValidationEnabled() const noexcept
  {
    return _isValidationEnabled;
  }

  inline bool VKRLib::isDebugEnabled() const noexcept
  {
    return _isDebugEnabled;
  }

  inline bool VKRLib::isLayerSupported(const char* layerName)
  {
    for (const VkLayerProperties& layer : availableLayers())
    {
      if (strcmp(layerName, layer.layerName) == 0) return true;
    }
    return false;
  }

  inline bool VKRLib::isExtensionSupported(const char* extensionName)
  {
    for (const VkExtensionProperties& extension : availableExtensions())
    {
      if (strcmp(extensionName, extension.extensionName) == 0) return true;
    }
    return false;
  }

  inline std::vector<PhysicalDevice*> VKRLib::devices() const
  {
    std::vector<PhysicalDevice*> result;
    for(const std::unique_ptr<PhysicalDevice>& device : _devices)
    {
      result.push_back(device.get());
    }
    return result;
  }
}