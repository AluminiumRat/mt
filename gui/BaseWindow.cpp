#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>

#include <gui/BaseWindow.h>
#include <gui/GUILib.h>
#include <gui/WindowConfiguration.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vkr/Win32WindowSurface.h>

using namespace mt;

BaseWindow::BaseWindow(const char* name) :
  _handle(nullptr),
  _platformDescriptor(0),
  _size(800,600),
  _storedSize(_size),
  _isMinimized(false),
  _isMaximized(false),
  _name(name)
{
  try
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _handle = glfwCreateWindow(_size.x, _size.y, name, nullptr, nullptr);
    glfwSetWindowUserPointer(_handle, this);
    glfwSetWindowPosCallback(_handle, &BaseWindow::_moveHandler);
    glfwSetWindowSizeCallback(_handle, &BaseWindow::_resizeHandler);
    glfwSetWindowIconifyCallback(_handle, &BaseWindow::_iconifyHandler);
    glfwSetWindowMaximizeCallback(_handle, &BaseWindow::_maximizeHandler);

    glfwGetWindowPos(_handle, &_position.x, &_position.y);

    _platformDescriptor = (uint64_t)glfwGetWin32Window(_handle);

    GUILib::instance().registerWindow(*this);
  }
  catch(...)
  {
    cleanup();
    throw;
  }
}

BaseWindow::~BaseWindow() noexcept
{
  cleanup();
}

void BaseWindow::cleanup() noexcept
{
  if(_handle != nullptr)
  {
    glfwDestroyWindow(_handle);
    _handle = nullptr;
  }
  GUILib::instance().unregisterWindow(*this);
}

void BaseWindow::onClose() noexcept
{
}

void BaseWindow::close() noexcept
{
  if (isClosed()) return;

  try
  {
    saveConfiguration();
  }
  catch (std::exception& error)
  {
    Log::error() << "Unable to save window configuration: " << _name << ": " << error.what();
    Abort("Unable to save window configuration");
  }

  onClose();

  glfwDestroyWindow(_handle);
  _handle = nullptr;
}

void BaseWindow::update()
{
  if(isClosed()) return;

  if(glfwWindowShouldClose(_handle))
  {
    close();
  }
}

void BaseWindow::draw()
{
}

void BaseWindow::move(glm::ivec2 newPosition) noexcept
{
  if (isClosed()) return;
  glfwSetWindowPos(_handle, newPosition.x, newPosition.y);
}

void BaseWindow::_moveHandler(GLFWwindow* window, int xPos, int yPos)
{
  BaseWindow* baseWindow = (BaseWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(baseWindow != nullptr);

  if (baseWindow->isClosed()) return;
  baseWindow->_position = glm::uvec2(xPos, yPos);

  baseWindow->onMove();
}

void BaseWindow::onMove() noexcept
{
}

void BaseWindow::resize(glm::uvec2 newSize) noexcept
{
  if (isClosed()) return;
  glfwSetWindowSize(_handle, newSize.x, newSize.y);
}

void BaseWindow::_resizeHandler(GLFWwindow* window, int width, int height)
{
  BaseWindow* baseWindow = (BaseWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(baseWindow != nullptr);

  if(baseWindow->isClosed()) return;
  baseWindow->_size = glm::uvec2(width, height);

  if(!baseWindow->_isMinimized && !baseWindow->_isMaximized)
  {
    baseWindow->_storedSize = baseWindow->_size;
  }

  baseWindow->onResize();
}

void BaseWindow::onResize() noexcept
{
}

void BaseWindow::minimize() noexcept
{
  if (isClosed()) return;
  glfwIconifyWindow(_handle);
}

void BaseWindow::_iconifyHandler(GLFWwindow* window, int iconified)
{
  BaseWindow* baseWindow = (BaseWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(baseWindow != nullptr);
  if (baseWindow->isClosed()) return;
  baseWindow->_isMinimized = iconified;
}

void BaseWindow::maximize() noexcept
{
  if (isClosed()) return;
  glfwMaximizeWindow(_handle);
}

void BaseWindow::_maximizeHandler(GLFWwindow* window, int maximized)
{
  BaseWindow* baseWindow = (BaseWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(baseWindow != nullptr);
  if (baseWindow->isClosed()) return;
  baseWindow->_isMaximized = maximized;
}

void BaseWindow::saveConfiguration() const
{
  if(isClosed()) return;
  if(_name.empty()) return;

  WindowConfiguration configuration{};
  fillConfiguration(configuration);
  GUILib::instance().addConfiguration(_name, configuration);
}

void BaseWindow::fillConfiguration(WindowConfiguration& configuration) const
{
  configuration.xPos = _position.x;
  configuration.yPos = _position.y;
  configuration.width = _storedSize.x;
  configuration.height = _storedSize.y;
  configuration.isMinimized = _isMinimized;
  configuration.isMaximized = _isMaximized;
}

void BaseWindow::loadConfiguration()
{
  MT_ASSERT(!isClosed());
  if (_name.empty()) return;

  const WindowConfiguration* configuration =
                                    GUILib::instance().getConfiguration(_name);
  if(configuration == nullptr) return;

  applyConfiguration(*configuration);
}

void BaseWindow::applyConfiguration(const WindowConfiguration& configuration)
{
  // Размер
  glm::uvec2 newSize(configuration.width, configuration.height);
  newSize = glm::clamp(newSize, glm::uvec2(0), glm::uvec2(16535));
  resize(newSize);

  // Позиция
  glm::ivec2 newPosition(configuration.xPos, configuration.yPos);
  // Обойдем все мониторы и проверим, что окно пересекается хотябы с одним
  bool isPositionCorrect = false;
  int monitorsCount = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&monitorsCount);
  if (monitors != nullptr)
  {
    for(int i = 0; i < monitorsCount; i++)
    {
      int xPos = 0;
      int yPos = 0;
      int width = 0;
      int height = 0;
      glfwGetMonitorWorkarea(monitors[i], &xPos, &yPos, &width, &height);
      if( newPosition.x < (xPos + width) &&
          (newPosition.x + int(newSize.x)) > xPos &&
          newPosition.y < (yPos + height) &&
          (newPosition.y + int(newSize.y)) > yPos)
      {
        isPositionCorrect = true;
        break;
      }
    }
  }
  if(isPositionCorrect) move(newPosition);

  // Минимизация и максимизация
  if(configuration.isMinimized)
  {
    if(!_isMinimized) minimize();
  }
  else if(configuration.isMaximized)
  {
    if(!_isMaximized) maximize();
  }
  else
  {
    if(_isMinimized || _isMaximized) glfwRestoreWindow(_handle);
  }
}
