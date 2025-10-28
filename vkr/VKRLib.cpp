#include <algorithm>

#include <cstdlib>
#include <vector>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vkr/queue/QueueSources.h>
#include <vkr/VKRLib.h>

using namespace mt;

static const std::vector<const char*> requiredInstanceExtensions =
                                                  { "VK_KHR_surface",
                                                    "VK_KHR_win32_surface"};

static const char* debugExtensionName = "VK_EXT_debug_utils";
static const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

static const std::vector<const char*> requiredDeviceExtensions = {};
static const char* swapchainExtensionName = "VK_KHR_swapchain";

VKRLib* VKRLib::_instance = nullptr;

VKRLib::VKRLib( const char* applicationName,
                const AppVersion& applicationVersion,
                uint32_t vulkanAPIVersion,
                bool enableValidation,
                bool enableDebug) :
  _handle(VK_NULL_HANDLE),
  _debugMessenger(VK_NULL_HANDLE),
  _isValidationEnabled(enableValidation),
  _isDebugEnabled(enableDebug),
  _vulkanApiVersion(vulkanAPIVersion)
{
  MT_ASSERT((_vulkanApiVersion >= minimalVulkanAPIVersion) && "VKRLib::VKRLib: You can't use vulkanAPIVersion under VKRLib::minimalVulkanAPIVersion");
  MT_ASSERT((_instance == nullptr) && "VKRLib::VKRLib: VKRLib is already initialized")

  try
  {
    _createVKInstance(applicationName, applicationVersion);
    _setupDebugMessenger();
    _receivePhysicalDevices();
  }
  catch(...)
  {
    _cleanup();
    throw;
  }

  _instance = this;
}

bool VKRLib::isValidationSupported()
{
  // Проверяем, что доступен лэйер валидации. Также нам нужно расширение
  // VK_EXT_debug_utils для перехвата сообщений
  return  isLayerSupported(validationLayerName) &&
          isExtensionSupported(debugExtensionName);
}

bool VKRLib::isDebugSupported()
{
  // Проверяем, что доступно расширение VK_EXT_debug_utils
  return isExtensionSupported(debugExtensionName);
}

void VKRLib::_createVKInstance( const char* applicationName,
                                const AppVersion& applicationVersion)
{
  if (_isValidationEnabled && !isValidationSupported())
  {
    throw std::runtime_error("VKRLib: Validation is not supported.");
  }

  if (_isDebugEnabled && !isDebugSupported())
  {
    throw std::runtime_error("VKRLib: Debug is not supported.");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = applicationName;
  appInfo.applicationVersion = VK_MAKE_VERSION( applicationVersion.major,
                                                applicationVersion.minor,
                                                applicationVersion.patch);
  appInfo.pEngineName = "Renderer engine";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = _vulkanApiVersion;
  
  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  // Собираем необходимые расширения уровня instance
  std::vector<const char*> extensions = requiredInstanceExtensions;
  // VK_EXT_debug_utils нужно как для дебаг маркеров, так и для получения
  // фидбэка от лэйера валидации
  if(_isDebugEnabled || _isValidationEnabled)
  {
    extensions.push_back(debugExtensionName);
  }

  createInfo.enabledExtensionCount = uint32_t(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (_isValidationEnabled)
  {
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = &validationLayerName;

    _populateDebugInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else createInfo.enabledLayerCount = 0;

  if (vkCreateInstance(&createInfo, nullptr, &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Failed to create vk instance.");
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                      void* pUserData)
{
  if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    Log::info() << "[Vulkan validation] " << pCallbackData->pMessage;
  }
  else if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    Log::warning() << "[Vulkan validation] " << pCallbackData->pMessage;
  }
  else
  {
    Log::error() << "[Vulkan validation] " << pCallbackData->pMessage;
  }
  return VK_FALSE;
}

void VKRLib::_populateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity =
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  createInfo.pfnUserCallback = debugCallback;
}

void VKRLib::_setupDebugMessenger()
{
  if (!_isValidationEnabled) return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  _populateDebugInfo(createInfo);

  PFN_vkCreateDebugUtilsMessengerEXT createFunc =
    (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                                            _handle,
                                            "vkCreateDebugUtilsMessengerEXT");
  if(createFunc == nullptr)
  {
    throw std::runtime_error("VKRLib: vkCreateDebugUtilsMessengerEXT is not supported.");
  }

  if (createFunc( _handle,
                  &createInfo,
                  nullptr,
                  &_debugMessenger) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to create debug messenger.");
  }
}

void VKRLib::_receivePhysicalDevices()
{
  uint32_t deviceCount = 0;
  if(vkEnumeratePhysicalDevices(_handle, &deviceCount, nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to get physical device count.");
  }

  if(deviceCount == 0) return;

  _devices.reserve(deviceCount);

  std::vector<VkPhysicalDevice> devices(deviceCount);
  if(vkEnumeratePhysicalDevices(_handle,
                                &deviceCount,
                                devices.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to enumerate physical devices.");
  }

  for(VkPhysicalDevice deviceHandle : devices)
  {
    try
    {
      std::unique_ptr<PhysicalDevice> newDevice(
                                              new PhysicalDevice(deviceHandle));
      _devices.push_back(std::move(newDevice));
    }
    catch(const std::runtime_error& error)
    {
      Log::warning() << "VKRLib: Unable to access to physical device. Reason: " << error.what();
    }
  }
}

VKRLib::~VKRLib() noexcept
{
  _cleanup();
}

void VKRLib::_cleanup() noexcept
{
  if (_handle != VK_NULL_HANDLE)
  {
    if(_debugMessenger != VK_NULL_HANDLE)
    {
      PFN_vkDestroyDebugUtilsMessengerEXT destructFunc =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                              _handle,
                                              "vkDestroyDebugUtilsMessengerEXT");
      if (destructFunc != nullptr)
      {
        destructFunc(_handle, _debugMessenger, nullptr);
      }
    }

    vkDestroyInstance(_handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }

  _instance = nullptr;
}

std::vector<VkLayerProperties> VKRLib::availableLayers()
{
  uint32_t layerCount = 0;
  if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to get vk layers count.");
  }

  if (layerCount == 0) return {};

  std::vector<VkLayerProperties> availableLayers(layerCount);
  if (vkEnumerateInstanceLayerProperties(&layerCount,
      availableLayers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to enumerate vk layers.");
  }

  return availableLayers;
}

std::vector<VkExtensionProperties> VKRLib::availableExtensions()
{
  uint32_t extensionsCount = 0;
  if (vkEnumerateInstanceExtensionProperties(
        nullptr, &extensionsCount, nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to get vk extensions count.");
  }

  if (extensionsCount == 0) return {};

  std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
  if (vkEnumerateInstanceExtensionProperties(
        nullptr, &extensionsCount, availableExtensions.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("VKRLib: Unable to enumerate vk extensions.");
  }

  return availableExtensions;
}

void VKRLib::_extendRequiredFeatures(
                              VkPhysicalDeviceFeatures& requiredFeatures) const
{
  requiredFeatures.samplerAnisotropy = VK_TRUE;
  requiredFeatures.textureCompressionBC = VK_TRUE;
}

std::set<std::string> VKRLib::_extendRequiredExtensions(
                            const std::vector<std::string>& requiredExtensions,
                            bool needPresent) const
{
  std::set<std::string> extensions( requiredExtensions.begin(),
                                    requiredExtensions.end());
  if(needPresent) extensions.insert(swapchainExtensionName);
  for(const char* extension : requiredDeviceExtensions)
  {
    extensions.insert(extension);
  }
  return extensions;
}

bool VKRLib::_isDeviceSuitable(
  const PhysicalDevice& device,
  const VkPhysicalDeviceFeatures& requiredFeatures,
  const std::set<std::string>& requiredExtensions,
  bool requireGraphic,
  bool requireCompute,
  const WindowSurface* testSurface)
{
  // Для начала проверяем, что карта может отрисовывать в окно
  if (testSurface != nullptr && !device.isSurfaceSuitable(*testSurface))
  {
    Log::info() << "Device " << device.properties().deviceName << " is not suitable with window's surface";
    return false;
  }

  // Проверяем, что карта поддерживает все требуемые виды очередей
  // makeTransfer в false, так как трансфер всегда можно найти
  if (!makeQueueSources(device.queuesInfo(),
                        requireGraphic,
                        requireCompute,
                        false,            // makeTransfer
                        testSurface).has_value())
  {
    Log::info() << "Device " << device.properties().deviceName << " doesn't support all required queue types";
    return false;
  }

  // Проверяем, что карта поддерживает все требуемые фичи
  if (!device.areFeaturesSupported(requiredFeatures))
  {
    Log::info() << "Device " << device.properties().deviceName << " doesn't support all features";
    return false;
  }

  // Обязательно должны поддерживаться таймлайн семафоры, на них построена
  // синхронизация очередей
  if (!device.areTimelineSemaphoresSupported())
  {
    Log::info() << "Device " << device.properties().deviceName << " doesn't support timeline semaphores";
    return false;
  }

  if (!device.isSynchronization2Supported())
  {
    Log::info() << "Device " << device.properties().deviceName << " doesn't support synchronization2";
    return false;
  }

  // Проверяем, что карта поддерживает все требуемые расширения
  for (const std::string& extensionName : requiredExtensions)
  {
    if (!device.isExtensionSupported(extensionName.c_str()))
    {
      Log::info() << "Device " << device.properties().deviceName << " doesn't support extension " << extensionName;
      return false;
    }
  }

  return true;
}

PhysicalDevice* VKRLib::getBestPhysicalDevice(
                      VkPhysicalDeviceFeatures requiredFeatures,
                      const std::vector<std::string>& requiredExtensions,
                      bool requireGraphic,
                      bool requireCompute,
                      const WindowSurface* testSurface) const
{
  // Формируем список расширений, которые должны поддерживаться устройством
  std::set<std::string> extensions = _extendRequiredExtensions(
                                                      requiredExtensions,
                                                      testSurface != nullptr);

  // Дополняем пользовательские фичи требованиями движка
  _extendRequiredFeatures(requiredFeatures);

  // Выбираем видюху, которая имеет наибольший объем недоступной с CPU
  // памяти (дискретка). Если таких нет, то выбираем встройку
  PhysicalDevice* bestDevice = nullptr;
  VkDeviceSize bestMemorySize = 0;
  for(const std::unique_ptr<PhysicalDevice>& device : _devices)
  {
    if(!_isDeviceSuitable(*device,
                          requiredFeatures,
                          extensions,
                          requireGraphic,
                          requireCompute,
                          testSurface))
    {
      continue;
    }

    // Обходим все типы памяти и пытаемся определить, сколько есть честной
    // GPU памяти
    VkDeviceSize gpuMemorySize = 0;
    for(const MemoryTypeInfo& memoryType : device->memoryInfo().types)
    {
      if(memoryType.isDeviceOnly())
      {
        gpuMemorySize = std::max(gpuMemorySize, memoryType.heap->size);
      }
    }

    // Если это первый обнаруженный девайс, то нам нет разницы - встройка это или
    // нет, мы берем его как лучшего кандидата, так как лучше у нас всё равно
    // пока нету.
    // Если это не первый девайс, то выбираем тот у кого памяти больше. Встройки
    // отпадают автоматом, так как у них будет 0 памяти.
    if(bestDevice == nullptr || gpuMemorySize > bestMemorySize)
    {
      bestDevice = device.get();
      bestMemorySize = gpuMemorySize;
    }
  }

  return bestDevice;
}

std::unique_ptr<Device> VKRLib::createDevice(
  const VkPhysicalDeviceFeatures& requiredFeatures,
  const std::vector<std::string>& requiredExtensions,
  bool createGraphicQueue,
  bool createComputeQueue,
  bool createTransferQueue,
  const WindowSurface* testSurface) const
{
  PhysicalDevice* physicalDevice = getBestPhysicalDevice( requiredFeatures,
                                                          requiredExtensions,
                                                          createGraphicQueue,
                                                          createComputeQueue,
                                                          testSurface);
  if(physicalDevice == nullptr) return nullptr;

  return createDevice(*physicalDevice,
                      requiredFeatures,
                      requiredExtensions,
                      createGraphicQueue,
                      createComputeQueue,
                      createTransferQueue,
                      testSurface);
}

std::unique_ptr<Device> VKRLib::createDevice(
  PhysicalDevice& physicalDevice,
  VkPhysicalDeviceFeatures requiredFeatures,
  const std::vector<std::string>& enabledExtensions,
  bool createGraphicQueue,
  bool createComputeQueue,
  bool createTransferQueue,
  const WindowSurface* testSurface) const
{
  // Формируем список расширений, которые должны поддерживаться устройством
  std::set<std::string> extensionsSet = _extendRequiredExtensions(
                                                      enabledExtensions,
                                                      testSurface != nullptr);
  std::vector<std::string> extensions(extensionsSet.begin(),
                                      extensionsSet.end());

  // Дополняем пользовательские фичи требованиями движка
  _extendRequiredFeatures(requiredFeatures);

  // Список лэйеров, включенных на устройстве
  std::vector<std::string> enabledLayers;
  if(_isValidationEnabled) enabledLayers.push_back(validationLayerName);

  // Распределяем очереди
  std::optional<QueueSources> queueSources = makeQueueSources(
                                                    physicalDevice.queuesInfo(),
                                                    createGraphicQueue,
                                                    createComputeQueue,
                                                    createTransferQueue,
                                                    testSurface);
  MT_ASSERT(queueSources.has_value() && "VKRLib::createDevice: unable to create queue sources")

  return std::unique_ptr<Device>(new Device(physicalDevice,
                                            requiredFeatures,
                                            extensions,
                                            enabledLayers,
                                            *queueSources));
}