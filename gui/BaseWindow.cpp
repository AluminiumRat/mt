#include <util/Assert.h>

#include <gui/BaseWindow.h>
#include <gui/GUILib.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vkr/Win32WindowSurface.h>

using namespace mt;

BaseWindow::BaseWindow(const char* name) :
  _handle(nullptr),
  _size(800,600),
  _name(name)
{
  try
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _handle = glfwCreateWindow(_size.x, _size.y, name, nullptr, nullptr);
    glfwSetWindowUserPointer(_handle, this);
    glfwSetWindowSizeCallback(_handle, &BaseWindow::_resizeHandler);

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

void BaseWindow::_resizeHandler(GLFWwindow* window, int width, int height)
{
  BaseWindow* baseWindow = (BaseWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(baseWindow != nullptr);

  if(baseWindow->isClosed()) return;
  baseWindow->_size = glm::uvec2(width, height);
  baseWindow->onResize();
}

void BaseWindow::onResize() noexcept
{
}
