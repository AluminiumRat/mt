#include <glm/gtc/matrix_transform.hpp>

#include <util/Camera.h>

#include <FlatCameraManipulator.h>

FlatCameraManipulator::FlatCameraManipulator(mt::Camera& targetCamera) :
  _targetCamera(targetCamera),
  _frustumOrigin(0, 0),
  _frustumSize(1, 1),
  _minZ(-1000),
  _maxZ(1000)
{
}

void FlatCameraManipulator::update( ImVec2 areaPosition,
                                    ImVec2 areaSize) noexcept
{
  CameraManipulator::update(areaPosition, areaSize);
  _processMouseWheel();
  _aspectRatioCorrection();
  _updateCamera();
}

void FlatCameraManipulator::_processMouseWheel() noexcept
{
  if(areaSize().x == 0 || areaSize().y == 0) return;

  ImGuiIO& io = ImGui::GetIO();
  if(io.WantCaptureMouse) return;

  float scale = 1.0f - 0.1f * io.MouseWheel;

  glm::vec2 newFrustumSize = _frustumSize * scale;
  glm::vec2 frustumSizeDiff = newFrustumSize - _frustumSize;
  _frustumSize = newFrustumSize;

  glm::vec2 relMousePosition(mousePosition());
  relMousePosition /= areaSize();

  _frustumOrigin -= frustumSizeDiff * relMousePosition;
}

void FlatCameraManipulator::_aspectRatioCorrection() noexcept
{
  if (areaSize().x == 0) return;

  float aspectRatio = (float)areaSize().y / areaSize().x;
  _frustumSize.y = _frustumSize.x * aspectRatio;
}

void FlatCameraManipulator::_updateCamera() noexcept
{
  glm::vec2 halfFrustumSize = _frustumSize / 2.0f;
  glm::vec3 frustumCenter = glm::vec3(_frustumOrigin + halfFrustumSize,
                                      _maxZ);

  glm::mat4 translate(1);
  translate = glm::translate(translate, frustumCenter);
  _targetCamera.setTransformMatrix(translate);

  _targetCamera.setOrthoProjection( -halfFrustumSize.x,
                                    halfFrustumSize.x,
                                    halfFrustumSize.y,
                                    -halfFrustumSize.y,
                                    0,
                                    _maxZ - _minZ);
}

void FlatCameraManipulator::onAreaResized(glm::ivec2 oldSize,
                                          glm::ivec2 newSize)
{
  CameraManipulator::onAreaResized(oldSize, newSize);

  float updateRate = (float)newSize.x / oldSize.x;
  _frustumSize *= updateRate;
}

void FlatCameraManipulator::onDragging(glm::ivec2 mouseDelta)
{
  CameraManipulator::onDragging(mouseDelta);

  glm::vec2 fAreaSize(areaSize());
  if(fAreaSize.x == 0 || fAreaSize.y == 0) return;

  glm::vec2 fDelta(mouseDelta);
  fDelta /= fAreaSize;

  _frustumOrigin -= fDelta * _frustumSize;
}
