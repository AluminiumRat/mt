#pragma once

#include <string>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace mt
{
  struct WindowConfiguration;

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

    //  Окно открыто и имеет не нулевой размер
    inline bool isVisible() const noexcept;

    virtual void update();
    virtual void draw();

    inline const std::string& name() const noexcept;

    //  Координаты левого верхнего угла окна (вместе с рамками)
    inline glm::ivec2 position() const noexcept;
    //  Переместить окно. newPosition - координаты левого верхнего угла окна
    //    (вместе с рамками)
    void move(glm::ivec2 newPosition) noexcept;

    //  Размер активной области для рисования. То есть сюда не включены рамки,
    //  кнопки и тайтл бар.
    inline glm::uvec2 size() const noexcept;
    //  Изменить размер. Указывается размер активной области для рисования.
    //  То есть сюда не включены рамки, кнопки и тайтл бар.
    void resize(glm::uvec2 newSize) noexcept;

    inline bool isMinimized() const noexcept;
    void minimize() noexcept;

    inline bool isMaximized() const noexcept;
    void maximize() noexcept;

    //  Сохранить положение и размер окна для воостановления при следующем
    //    открытии. При сохранении и восстановлении окна отличают конфигурации
    //    друг друга по имени окна. Если вы используете loadConfiguration,
    //    то позабодьтесь о том, чтобы окна имели уникальные имена.
    //  Этот метод автоматически вызывается при закрытии окна, в деструкторе и
    //    в GUILib::saveConfiguration, так что, как правило, нет необходимости
    //    вызывать его вручную
    void saveConfiguration() const;

    //  Получить ранее сохраненную конфигурацию и выставить по ней окно
    //  При сохранении и восстановлении окна отличают конфигурации друг друга
    //    по имени окна. Если вы используете этот метод, то позабодьтесь о том,
    //    чтобы окна имели уникальные имена.
    void loadConfiguration();

    //  HWND для Windows
    inline uint64_t platformDescriptor() const noexcept;

  protected:
    void cleanup() noexcept;

    inline GLFWwindow& handle() const noexcept;

    //  Обработчик изменения позиции окна.
    //  К моменту вызова этого обработчика координаты окна уже изменились
    virtual void onMove() noexcept;

    //  Обработчик изменения размеров.
    //  К моменту вызова этого обработчика размеры окна уже изменились
    virtual void onResize() noexcept;

    //  Обработчик закрытия окна
    //  Вызывается непосредственно перед glfwDestroyWindow
    //  Не вызывается при закрытии окна в деструкторе класса
    virtual void onClose() noexcept;

    //  Настроить конфигурацию окна перед сохранением
    virtual void fillConfiguration(WindowConfiguration& configuration) const;
    //  Применить конфигурацию после загрузки
    virtual void applyConfiguration(const WindowConfiguration& configuration);

  private:
    static void _moveHandler(GLFWwindow* window, int xPos, int yPos);
    static void _resizeHandler(GLFWwindow* window, int width, int height);
    static void _iconifyHandler(GLFWwindow* window, int iconified);
    static void _maximizeHandler(GLFWwindow* window, int maximized);

  private:
    GLFWwindow* _handle;
    uint64_t _platformDescriptor;

    //  Верхний левый край окна
    glm::ivec2 _position;

    //  Актуальный размер области отрисовки
    glm::uvec2 _size;
    //  Размер области отрисовки в обычном режиме (не свернуто и не развернуто
    //  на весь экран)
    glm::uvec2 _storedSize;

    bool _isMinimized;
    bool _isMaximized;
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

  inline glm::ivec2 BaseWindow::position() const noexcept
  {
    return _position;
  }

  inline glm::uvec2 BaseWindow::size() const noexcept
  {
    return _size;
  }

  inline bool BaseWindow::isMinimized() const noexcept
  {
    return _isMinimized;
  }

  inline bool BaseWindow::isMaximized() const noexcept
  {
    return _isMaximized;
  }

  inline GLFWwindow& BaseWindow::handle() const noexcept
  {
    return *_handle;
  }

  inline uint64_t BaseWindow::platformDescriptor() const noexcept
  {
    return _platformDescriptor;
  }
}