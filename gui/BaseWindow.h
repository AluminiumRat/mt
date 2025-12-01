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

    virtual void update();
    virtual void draw();

    inline const std::string& name() const noexcept;

    // Размер активной области для рисования. То есть сюда не включены рамки,
    // кнопки и тайтл бар.
    inline glm::vec2 size() const noexcept;

    // Пользователь закрыл окно
    bool shouldClose() const noexcept;

  protected:
    void cleanup() noexcept;

    inline GLFWwindow& handle() const noexcept;

    //  Обработчик изменения размеров. К моменту вызова этого обработчика
    //  все команды из очередей GPU будут выполнены, а сами очереди
    //  заблокированы от доступа из других потоков.
    virtual void onResize() noexcept;

  private:
    static void _resizeHandler(GLFWwindow* window, int width, int height);

  private:
    GLFWwindow* _handle;
    glm::uvec2 _size;
    std::string _name;
  };

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