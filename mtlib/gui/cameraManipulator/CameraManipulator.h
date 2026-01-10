#pragma once

#include <glm/glm.hpp>

#include <imgui.h>

namespace mt
{
  class Camera;

  //  Манипуляторы камеры - это объекты, которые перехватывают ввод от ImGui
  //    и на его основе управляют положением и настройками камеры.
  //  CameraManipulator - эбстрактный класс, который преобразовавает ввод ImGui
  //    в события, удобные для управления камерой. Конкретные реализации
  //    манипуляторов наследуются от этого класса
  class CameraManipulator
  {
  public:
    //  Где находится область, над которой должен работать манипулятор
    enum Location
    {
      //  Манипулятор работает над окном приложения, но вне области
      //  окон ImGui.
      APPLICATION_WINDOW_LOCATION,
      //  Манипулятор встроен в окно ImGui. Метод update должен вызываться
      //  внутри ImGui::Begin(), ImGui::End()
      IMGUI_WINDOW_LOCATION
    };

  public:
    explicit CameraManipulator(Location location);
    CameraManipulator(const CameraManipulator&) = delete;
    CameraManipulator& operator = (const CameraManipulator&) = delete;
    virtual ~CameraManipulator() noexcept = default;

    inline Location location() const noexcept;

    virtual void setCamera(Camera* newCamera);
    inline Camera* camera() const noexcept;

    //  Необходимо вызывать каждый кадр. При этом активный ImGui контекст должен
    //    быть не нулевой. Проще всего - вызывать во время GUI обхода окна.
    //  areaPosition и areaSize - положение(левый верхний угол) и размер
    //    области, над которой работает манипулятор. Координаты в ImGui скрин
    //    координатах (ImGui::GetCursorScreenPos)
    virtual void update(ImVec2 areaPosition, ImVec2 areaSize) noexcept;

    //  Манипулятор находится в активном состоянии, то есть перехватывает
    //  сообщения ввода и управляет камерой
    inline bool isActive() const noexcept;

  protected:
    enum DraggingState
    {
      NO_DRAGGING,
      DRAGGING
    };

  protected:
    //  позиция в ImGui скрин координатах (ImGui::GetCursorScreenPos)
    inline glm::ivec2 areaPosition() const noexcept;
    inline glm::ivec2 areaSize() const noexcept;
    //  позиция курсора относительно левого верхнего угла области манипулятора
    inline glm::ivec2 mousePosition() const noexcept;
    inline DraggingState draggingState() const noexcept;

  protected:
    //  Вызывается в первом update, когда положение и размер области манипулятора
    //    выставляются в первый раз
    //  position в ImGui скрин координатах (ImGui::GetCursorScreenPos)
    virtual void onAreaInitialized(glm::ivec2 position, glm::ivec2 size);

    //  Координаты в ImGui скрин координатах (ImGui::GetCursorScreenPos)
    virtual void onAreaMoved(glm::ivec2 oldPosition, glm::ivec2 newPosition);
    virtual void onAreaResized(glm::ivec2 oldSize, glm::ivec2 newSize);

    //  startMousePosition - позиция курсора относительно левого верхнего угла
    //    области манипулятора
    virtual void onDraggingStarted(glm::ivec2 startMousePosition);

    //  mouseDelta - насколько сместился курсор мыши
    virtual void onDragging(glm::ivec2 mouseDelta);

    virtual void onDraggingFinished();

  private:
    void _processMoveResize(ImVec2 areaPosition, ImVec2 areaSize) noexcept;
    void _updateIsActive() noexcept;
    void _processMouse() noexcept;

  private:
    Location _location;

    Camera* _camera;

    //  Флаг, говорящий о том, что update вызывается не первыйц раз.
    //  Нужен чтобы не генерировать лишние события при первом update
    bool _areaInitialized;
    glm::ivec2 _areaPosition;
    glm::ivec2 _areaSize;

    //  Флаг, говорящий о том, что пользователь в данный момент работает с этим
    //  манипулятором
    bool _isActive;

    bool _mousePressed;
    glm::ivec2 _mousePosition;

    //  Флаг, обозначающий что пользователь уже нажал на кнопку мыши над
    //  областью манипулятора, но ещё не перемещал курсор
    bool _preDragging;

    DraggingState _draggingState;
  };

  inline CameraManipulator::Location
                                    CameraManipulator::location() const noexcept
  {
    return _location;
  }

  inline Camera* CameraManipulator::camera() const noexcept
  {
    return _camera;
  }

  inline bool CameraManipulator::isActive() const noexcept
  {
    return _isActive;
  }

  inline glm::ivec2 CameraManipulator::areaPosition() const noexcept
  {
    return _areaPosition;
  }

  inline glm::ivec2 CameraManipulator::areaSize() const noexcept
  {
    return _areaSize;
  }

  inline glm::ivec2 CameraManipulator::mousePosition() const noexcept
  {
    return _mousePosition;
  }

  inline CameraManipulator::DraggingState
                                CameraManipulator::draggingState() const noexcept
  {
    return _draggingState;
  }
}