#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include <gui/cameraManipulator/OrbitalCameraManipulator.h>
#include <util/Abort.h>
#include <util/Camera.h>
#include <util/pi.h>

using namespace mt;

OrbitalCameraManipulator::OrbitalCameraManipulator() :
  _centerPosition(0),
  _distance(5),
  _yawAngle(-glm::pi<float>() / 4.f),
  _pitchAngle(-glm::pi<float>() / 4.f),
  _fowY(pi / 4.0f),
  _nearPlane(0.1f),
  _farPlane(100.0f),
  _rotateSensitivity(2.f)
{
}

void OrbitalCameraManipulator::update(ImVec2 areaPosition,
                                      ImVec2 areaSize) noexcept
{
  CameraManipulator::update(areaPosition, areaSize);
  _processMouseWheel();
  _updateCameraPosition();
  _updateProjectionMatrix(areaSize);
}

void OrbitalCameraManipulator::_processMouseWheel() noexcept
{
  if (!isActive()) return;
  ImGuiIO& io = ImGui::GetIO();
  _distance *= 1.0f - 0.1f * io.MouseWheel;
  _distance = std::max(0.01f, _distance);
}

void OrbitalCameraManipulator::_updateCameraPosition() noexcept
{
  if(camera() == nullptr) return;

  glm::mat4 viewMatrix = glm::lookAt( glm::vec3(_distance, 0.f, 0.f),
                                      glm::vec3(0, 0, 0),
                                      glm::vec3(0, 0, 1));
  glm::mat4 cameraTransform = glm::inverse(viewMatrix);

  glm::mat4 rotateTransform = glm::eulerAngleZ(_yawAngle);
  rotateTransform = rotateTransform * glm::eulerAngleY(_pitchAngle);
  cameraTransform = rotateTransform * cameraTransform;

  glm::mat4 moveTransform = glm::translate(glm::mat4(1), _centerPosition);
  cameraTransform = moveTransform * cameraTransform;

  camera()->setTransformMatrix(cameraTransform);
}

void OrbitalCameraManipulator::onDragging(glm::ivec2 mouseDelta)
{
  CameraManipulator::onDragging(mouseDelta);

  glm::vec2 fAreaSize(areaSize());
  if (fAreaSize.x == 0 || fAreaSize.y == 0) return;

  glm::vec2 fDelta(mouseDelta);
  fDelta /= fAreaSize;

  _yawAngle -= fDelta.x * _rotateSensitivity;
  while (_yawAngle > pi) _yawAngle -= 2.0f * pi;
  while (_yawAngle < -pi) _yawAngle += 2.0f * pi;

  _pitchAngle -= fDelta.y * _rotateSensitivity;
  _pitchAngle = glm::clamp(_pitchAngle, -pi / 2.0f, pi / 2.0f);
}

void OrbitalCameraManipulator::_updateProjectionMatrix(ImVec2 areaSize) noexcept
{
  if (camera() == nullptr) return;
  if(areaSize.x == 0 || areaSize.y == 0) return;

  camera()->setPerspectiveProjection( _fowY,
                                      areaSize.x / areaSize.y,
                                      _nearPlane,
                                      _farPlane);
}
