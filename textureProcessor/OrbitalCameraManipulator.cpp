#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include <util/Abort.h>
#include <util/Camera.h>
#include <util/pi.h>

#include <OrbitalCameraManipulator.h>

OrbitalCameraManipulator::OrbitalCameraManipulator(mt::Camera& targetCamera) :
  _targetCamera(targetCamera),
  _centerPosition(0),
  _distance(5),
  _yawAngle(-glm::pi<float>() / 4.f),
  _pitchAngle(-glm::pi<float>() / 4.f),
  _fowY(mt::pi / 4.0f),
  _nearPlane(0.1f),
  _farPlane(100.0f),
  _rotateSensitivity(2.f)
{
}

void OrbitalCameraManipulator::update(ImVec2 areaPosition,
                                      ImVec2 areaSize) noexcept
{
  CameraManipulator::update(areaPosition, areaSize);
  _updateCameraPosition();
  _updateProjectionMatrix(areaSize);
}

void OrbitalCameraManipulator::_updateCameraPosition() noexcept
{
  glm::mat4 viewMatrix = glm::lookAt( glm::vec3(_distance, 0.f, 0.f),
                                      glm::vec3(0, 0, 0),
                                      glm::vec3(0, 0, 1));
  glm::mat4 cameraTransform = glm::inverse(viewMatrix);

  glm::mat4 rotateTransform = glm::eulerAngleZ(_yawAngle);
  rotateTransform = rotateTransform * glm::eulerAngleY(_pitchAngle);
  cameraTransform = rotateTransform * cameraTransform;

  glm::mat4 moveTransform = glm::translate(glm::mat4(1), _centerPosition);
  cameraTransform = moveTransform * cameraTransform;

  _targetCamera.setTransformMatrix(cameraTransform);
}

void OrbitalCameraManipulator::onDragging(glm::ivec2 mouseDelta)
{
  CameraManipulator::onDragging(mouseDelta);

  glm::vec2 fAreaSize(areaSize());
  if (fAreaSize.x == 0 || fAreaSize.y == 0) return;

  glm::vec2 fDelta(mouseDelta);
  fDelta /= fAreaSize;

  _yawAngle -= fDelta.x * _rotateSensitivity;
  while (_yawAngle > mt::pi) _yawAngle -= 2.0f * mt::pi;
  while (_yawAngle < -mt::pi) _yawAngle += 2.0f * mt::pi;

  _pitchAngle -= fDelta.y * _rotateSensitivity;
  _pitchAngle = glm::clamp(_pitchAngle, -mt::pi / 2.0f, mt::pi / 2.0f);
}

void OrbitalCameraManipulator::_updateProjectionMatrix(ImVec2 areaSize) noexcept
{
  if(areaSize.x == 0 || areaSize.y == 0) return;

  _targetCamera.setPerspectiveProjection( _fowY,
                                          areaSize.x / areaSize.y,
                                          _nearPlane,
                                          _farPlane);
}
