#include <stdexcept>

#include <gui/cameraManipulator/CameraManipulator.h>
#include <util/Log.h>

using namespace mt;

#define BEGIN_EXCEPT_CATCHING try{
#define END_EXCEPT_CATCHING } catch(std::exception& error) { Log::error() << error.what(); }

//#define LOG_MANIPULATOR_EVENTS

CameraManipulator::CameraManipulator() :
  _camera(nullptr),
  _areaInitialized(false),
  _areaPosition(0, 0),
  _areaSize(0, 0),
  _isActive(false),
  _mousePressed(false),
  _mousePosition(0,0),
  _preDragging(false),
  _draggingState(NO_DRAGGING)
{
}

void CameraManipulator::setCamera(Camera* newCamera)
{
  _camera = newCamera;
}

void CameraManipulator::update(ImVec2 areaPosition, ImVec2 areaSize) noexcept
{
  _processMoveResize(areaPosition, areaSize);
  _processMouse();
}

void CameraManipulator::_processMoveResize( ImVec2 areaPosition,
                                            ImVec2 areaSize) noexcept
{
  glm::ivec2 newAreaPosition( (uint32_t)areaPosition.x,
                              (uint32_t)areaPosition.y);
  glm::ivec2 newAreaSize( (uint32_t)areaSize.x, (uint32_t)areaSize.y);

  if(newAreaPosition != _areaPosition)
  {
    glm::ivec2 oldAreaPosition = _areaPosition;
    _areaPosition = newAreaPosition;
    if(_areaInitialized)
    {
      BEGIN_EXCEPT_CATCHING;
      onAreaMoved(oldAreaPosition, _areaPosition);
      END_EXCEPT_CATCHING;
    }
  }

  if(newAreaSize != _areaSize)
  {
    glm::ivec2 oldAreaSize = _areaSize;
    _areaSize = newAreaSize;
    if(_areaInitialized)
    {
      BEGIN_EXCEPT_CATCHING;
      onAreaResized(oldAreaSize, _areaSize);
      END_EXCEPT_CATCHING;
    }
  }

  if(!_areaInitialized)
  {
    _areaInitialized = true;
    onAreaInitialized(_areaPosition, _areaSize);
  }
}

void CameraManipulator::onAreaMoved(glm::ivec2 oldPosition,
                                    glm::ivec2 newPosition)
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onAreaMoved " << newPosition.x << " " << newPosition.y;
  #endif
}

void CameraManipulator::onAreaResized(glm::ivec2 oldSize, glm::ivec2 newSize)
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onAreaResized " << oldSize.x << " " << newSize.y;
  #endif
}

void CameraManipulator::onAreaInitialized(glm::ivec2 position, glm::ivec2 size)
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onAreaInitialized. Position: " << position.x << " " << position.y << " Size: " << size.x << " " << size.y;
  #endif
}

void CameraManipulator::_updateIsActive() noexcept
{
  //  В случае, если было начато перетаскивание, то манипулятор деактивируется
  //  только, когда пользователь отпустит кнопку мыши
  if(_draggingState == DRAGGING)
  {
    _isActive = true;
    return;
  }

  //  Если мышь вне области манипулятора - то манипулятор не активный
  if( _mousePosition.x < 0 ||
      _mousePosition.x > _areaSize.x ||
      _mousePosition.y < 0 ||
      _mousePosition.y > _areaSize.y)
  {
    _isActive = false;
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  if(io.WantCaptureMouse == false)
  {
    //  Случай, когда мышь находится над областью манипулятора, но при этом
    //  не находится над окнами ImGui. Манипулятор не привязан к окнам ImGui
    _isActive = true;
    return;
  }

  // Проверяем, активное ли окно, к которому привязан манипулятор.
  _isActive = ImGui::IsWindowFocused();
}

void CameraManipulator::_processMouse() noexcept
{
  ImGuiIO& io = ImGui::GetIO();

  bool lastMousePressed = _mousePressed;
  _mousePressed = ImGui::IsKeyDown(ImGuiKey_MouseLeft);

  glm::ivec2 lastMousePosition = _mousePosition;
  _mousePosition = glm::ivec2(io.MousePos.x, io.MousePos.y) - _areaPosition;

  _updateIsActive();

  if(_draggingState == NO_DRAGGING)
  {
    //  Состояние - нет перетаскивания. Здесь нам надо отследить момент, когда
    //  перетаскивание начинается. Это происходит, когда пользователь начинает
    //  перемещать мышь с зажатой кнопкой над областью манипулятора
    if(_isActive)
    {
      if(_mousePressed && !lastMousePressed)
      {
        //  Пользователь только что нажал кнопку над областью манипулятора
        _preDragging = true;
      }
      else if(_mousePressed && _preDragging)
      {
        //  Мышь уже нажата какое-то время, ждем когда курсор начнет смещаться
        if (lastMousePosition != _mousePosition)
        {
          _draggingState = DRAGGING;
          BEGIN_EXCEPT_CATCHING;
          onDraggingStarted(lastMousePosition);
          END_EXCEPT_CATCHING;

          BEGIN_EXCEPT_CATCHING;
          onDragging(_mousePosition - lastMousePosition);
          END_EXCEPT_CATCHING;

          _preDragging = false;
        }
      }
      else
      {
        // Мышь не нажата
        _preDragging = false;
      }
    }
  }
  else
  {
    //  Состояние - перетаскивание в процессе. Мониторим перемещение курсора
    //  и ждем, когда пользователь отпустит кнопку
    if(!_mousePressed)
    {
      _draggingState = NO_DRAGGING;
      BEGIN_EXCEPT_CATCHING;
      onDraggingFinished();
      END_EXCEPT_CATCHING;
    }
    else
    {
      if (lastMousePosition != _mousePosition)
      {
        BEGIN_EXCEPT_CATCHING;
        onDragging(_mousePosition - lastMousePosition);
        END_EXCEPT_CATCHING;
      }
    }
  }
}

void CameraManipulator::onDraggingStarted(glm::ivec2 startMousePosition)
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onDraggingStarted " << startMousePosition.x << " " << startMousePosition.y;
  #endif
}

void CameraManipulator::onDragging(glm::ivec2 mouseDelta)
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onDragging " << mouseDelta.x << " " << mouseDelta.y;
  #endif
}

void CameraManipulator::onDraggingFinished()
{
  #ifdef LOG_MANIPULATOR_EVENTS
    Log::info() << "onDraggingFinished";
  #endif
}
