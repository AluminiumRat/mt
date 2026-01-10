#include <set>
#include <stdexcept>

#include <util/Assert.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/SwapChain.h>
#include <vkr/WindowSurface.h>

using namespace mt;

// Выбрать формат для свапчейно. Ищем SRGB, иначе используем первый
// попавшийся формат
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                        const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  MT_ASSERT(!availableFormats.empty());
  for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

// Выбрать режим презентации. Если поддерживается VK_PRESENT_MODE_MAILBOX_KHR,
// то выбираем его. Иначе - VK_PRESENT_MODE_FIFO_KHR
static VkPresentModeKHR chooseSwapPresentMode(
                    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  MT_ASSERT(!availablePresentModes.empty());

  for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  else
  {
    VkExtent2D actualExtent = { 800, 600 };

    actualExtent.width = std::max(capabilities.minImageExtent.width,
                                  std::min( capabilities.maxImageExtent.width,
                                            actualExtent.width));
    actualExtent.height = std::max(
                                capabilities.minImageExtent.height,
                                std::min( capabilities.maxImageExtent.height,
                                          actualExtent.height));
    return actualExtent;
  }
}

static uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
  uint32_t imageCount = capabilities.minImageCount + 1;
  imageCount = std::max(imageCount, 3u);
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount)
  {
    imageCount = capabilities.maxImageCount;
  }
  return imageCount;
}

// По списку очередей получаем индексы их семейств
static std::vector<uint32_t> getFamilyIndices(
                                const std::vector<CommandQueue const*>& queues)
{
  std::set<uint32_t> indices;
  for(const CommandQueue* queue : queues)
  {
    indices.insert(queue->family().index());
  }

  return std::vector<uint32_t>(indices.begin(), indices.end());
}

SwapChain::SwapChain( Device& device,
                      WindowSurface& surface,
                      std::optional<VkPresentModeKHR> presentationMode,
                      std::optional<VkSurfaceFormatKHR> format) :
  _device(device),
  _presentQueue(device.presentationQueue()),
  _primaryQueue(&device.primaryQueue()),
  _handle(VK_NULL_HANDLE),
  _imageFormat{ .format = VK_FORMAT_UNDEFINED,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
  _extent(0, 0),
  _presentationMode(VK_PRESENT_MODE_FIFO_KHR),
  _imageIsReadyFence(new Fence(device))
{
  MT_ASSERT(_presentQueue != nullptr);

  try
  {
    _createHandle(surface, presentationMode, format);

    // Так как vkAcquireNextImageKHR требует на вход гарантированно
    // неиспользуемый семафор, то нам необходимо иметь по меньшей мере на 1
    // пару семафоров больше, чем кадров в свапчейне, так как мы заранее не
    // можем сказать, какой именно Image вернет свапчейн.
    _semaphoresPool.resize(_frames.size() + 1);
    for(SemaphoresPair& semaphoresPair : _semaphoresPool)
    {
      semaphoresPair.startDrawing = new Semaphore(_device);
      semaphoresPair.endDrawing = new Semaphore(_device);
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

void SwapChain::_createHandle(
  WindowSurface& surface,
  std::optional<VkPresentModeKHR> presentationMode,
  std::optional<VkSurfaceFormatKHR> format)
{
  MT_ASSERT(_device.physicalDevice().isSurfaceSuitable(surface));

  PhysicalDevice::SwapChainSupport swapChainSupport =
                      _device.physicalDevice().surfaceСompatibility(surface);

  _imageFormat = format.has_value() ?
                              *format :
                              chooseSwapSurfaceFormat(swapChainSupport.formats);

  _presentationMode = presentationMode.has_value() ?
                          *presentationMode :
                          chooseSwapPresentMode(swapChainSupport.presentModes);

  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
  _extent = glm::ivec2(extent.width, extent.height);

  uint32_t imageCount = chooseImageCount(swapChainSupport.capabilities);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface.handle();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = _imageFormat.format;
  createInfo.imageColorSpace = _imageFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  std::vector<CommandQueue const*> ownerQueues{_presentQueue, _primaryQueue};
  std::vector<uint32_t> familyIndices = getFamilyIndices(ownerQueues);
  if(familyIndices.size() <= 1)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = uint32_t(familyIndices.size());
    createInfo.pQueueFamilyIndices = familyIndices.data();
  }
  
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = _presentationMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR( _device.handle(),
                            &createInfo,
                            nullptr,
                            &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error("SwapChain: Unable to create swap chain.");
  }

  if(vkGetSwapchainImagesKHR( _device.handle(),
                              _handle,
                              &imageCount,
                              nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("SwapChain: Unable to get images count from swapchain.");
  }

  std::vector<VkImage> images(imageCount);
  if(vkGetSwapchainImagesKHR( _device.handle(),
                              _handle,
                              &imageCount,
                              images.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("SwapChain: Unable to get images from swapchain.");
  }

  _frames.resize(imageCount);
  for(size_t frameIndex = 0; frameIndex < _frames.size(); frameIndex++)
  {
    _frames[frameIndex].image = new Image(_device,
                                          images[frameIndex],
                                          VK_IMAGE_TYPE_2D,
                                          _imageFormat.format,
                                          glm::uvec3(_extent, 1),
                                          VK_SAMPLE_COUNT_1_BIT,
                                          1,
                                          1,
                                          createInfo.imageSharingMode,
                                          false,
                                          ImageAccess(),
                                          "Swapchain image");
  }
}

SwapChain::~SwapChain() noexcept
{
  MT_ASSERT(!_lockedFrameIndex.has_value() && "Swapchain in the locked state")
  _cleanup();
}

void SwapChain::_cleanup() noexcept
{
  _primaryQueue->waitIdle();
  _presentQueue->waitIdle();

  _frames.clear();

  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}

uint32_t SwapChain::lockFrame()
{
  MT_ASSERT(!_lockedFrameIndex.has_value() && "swapchain is already in the locked state");
  MT_ASSERT(!_semaphoresPool.empty());

  SemaphoresPair newSemaphores = _semaphoresPool.back();
  _semaphoresPool.pop_back();

  _imageIsReadyFence->reset();

  uint32_t frameIndex;
  if(vkAcquireNextImageKHR( _device.handle(),
                            _handle,
                            UINT64_MAX,
                            newSemaphores.startDrawing->handle(),
                            _imageIsReadyFence->handle(),
                            &frameIndex) != VK_SUCCESS)
  {
    throw std::runtime_error("SwapChain: Failed to acquire next image from swapchain.");
  }

  // Если к этому фрэйму уже были привязаны семафоры, то возвращаем их в пул
  if(_frames[frameIndex].semaphores.startDrawing != nullptr)
  {
    _semaphoresPool.push_back(_frames[frameIndex].semaphores);
  }
  // Привязываем новые семафоры к фрэйму
  _frames[frameIndex].semaphores = newSemaphores;

  _primaryQueue->addWaitForSemaphore( *newSemaphores.startDrawing,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  _lockedFrameIndex = frameIndex;
  return frameIndex;
}

void SwapChain::presentFrame()
{
  MT_ASSERT(_lockedFrameIndex.has_value() && "swapchain is not in the locked state");

  // Дожидаемся, когда картинка будет освобождена от предыдущих
  // манипуляций в свапчейне
  _imageIsReadyFence->wait();

  uint32_t frameIndex = _lockedFrameIndex.value();
  Semaphore& endDrawningSemaphore = *_frames[frameIndex].semaphores.endDrawing;

  // Для начала синхронизируемся с основной очередью, для того чтобы
  // дождаться окончания отрисовки.
  _primaryQueue->addSignalSemaphore(endDrawningSemaphore);

  // Теперь отправляем картинку на презентацию
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  VkSemaphore waitSemaphoreHandle = endDrawningSemaphore.handle();
  presentInfo.pWaitSemaphores = &waitSemaphoreHandle;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &_handle;
  presentInfo.pImageIndices = &frameIndex;
  presentInfo.pResults = nullptr;
  if(vkQueuePresentKHR(_presentQueue->handle(), &presentInfo) != VK_SUCCESS)
  {
    Log::warning() << "SwapChain::_unlockFrame: error in vkQueuePresentKHR";
  }

  _lockedFrameIndex.reset();
}

void SwapChain::unlockFrame() noexcept
{
  // Это аварийный анлок кадра. Делаем то же самое, что и при презентации,
  // просто в случае исключения абортимся, так как это пат, свапчейн сломан.
  try
  {
    presentFrame();
  }
  catch(std::exception& error)
  {
    mt::Abort(error.what());
  }
}
