#pragma once

#include <string>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace mt
{
  //  Базовая работа с окном приложения.
  //  Построен на GLFW
  //  GUILib должна быть инициализированная до создания первого окна
  class BaseWindow
  {
  public:
    BaseWindow(const char* name);
    BaseWindow(const BaseWindow&) = delete;
    BaseWindow& operator = (const BaseWindow&) = delete;
    virtual ~BaseWindow() noexcept;

    void close() noexcept;
    inline bool isClosed() const noexcept;

    // Окно открыто и имеет не нулевой размер
    inline bool isVisible() const noexcept;

    virtual void update();
    virtual void draw();

    inline const std::string& name() const noexcept;

    // Размер активной области для рисования. То есть сюда не включены рамки,
    // кнопки и тайтл бар.
    inline glm::vec2 size() const noexcept;

  protected:
    void cleanup() noexcept;

    inline GLFWwindow& handle() const noexcept;

    //  Обработчик изменения размеров.
    //  К моменту вызова этого обработчика размеры окна уже изменились
    virtual void onResize() noexcept;

    //  Обработчик закрытия окна
    //  Вызывается непосредственно перед glfwDestroyWindow
    //  Не вызывается при закрытии окна в деструкторе класса
    virtual void onClose() noexcept;

  private:
    static void _resizeHandler(GLFWwindow* window, int width, int height);

  private:
    GLFWwindow* _handle;
    glm::uvec2 _size;
    std::string _name;
  };

  inline bool BaseWindow::isClosed() const noexcept
  {
    return _handle == nullptr;
  }

  inline bool BaseWindow::isVisible() const noexcept
  {
    return !isClosed() && _size.x != 0 && _size.y != 0;
  }

  inline const std::string& BaseWindow::name() const noexcept
  {
    return _name;
  }

  inline glm::vec2 BaseWindow::size() const noexcept
  {
    return _size;
  }

  inline GLFWwindow& BaseWindow::handle() const noexcept
  {
    return *_handle;
  }
}