#include <glm/gtc/matrix_transform.hpp>

#include <util/Camera.h>

using namespace mt;

Camera::Camera() noexcept :
  _viewMatrix(1),
  _projectionMatrix(1),
  _nearDistance(0),
  _farDistance(1)
{
  _updateFromPositionMatrix();
  _updateFromProjectionMatrix();
}

void Camera::setTransformMatrix(const glm::mat4& newValue) noexcept
{
  _transformMatrix = newValue;
  _updateFromPositionMatrix();
}

void Camera::_updateFromPositionMatrix() noexcept
{
  _eyePoint = transformMatrix() * glm::vec4(0, 0, 0, 1);
  _frontVector = transformMatrix() * glm::vec4(0, 0, -1, 0);
  _frontVector = glm::normalize(_frontVector);
  _upVector = transformMatrix() * glm::vec4(0, 1, 0, 0);
  _upVector = glm::normalize(_upVector);
  _rightVector = transformMatrix() * glm::vec4(1, 0, 0, 0);
  _rightVector = glm::normalize(_rightVector);
  _viewMatrix = glm::inverse(transformMatrix());
}

void Camera::setPerspectiveProjection(float fovY,
                                      float aspect,
                                      float zNear,
                                      float zFar) noexcept
{
  setProjectionMatrix(projectionCorrect *
                                  glm::perspective(fovY, aspect, zNear, zFar));
}

void Camera::setOrthoProjection(float left,
                                float right,
                                float bottom,
                                float top,
                                float zNear,
                                float zFar) noexcept
{
  setProjectionMatrix(projectionCorrect *
                            glm::ortho(left, right, bottom, top, zNear, zFar));
}

void Camera::setProjectionMatrix(glm::mat4 newValue) noexcept
{
  _projectionMatrix = newValue;
  _updateFromProjectionMatrix();
}

void Camera::_updateFromProjectionMatrix() noexcept
{
  _inverseProjectionMatrix = glm::inverse(_projectionMatrix);

  glm::vec4 nearPoint = _inverseProjectionMatrix * glm::vec4(0, 0, 1, 1);
  _nearDistance = fabs(nearPoint.z / nearPoint.w);

  glm::vec4 farPoint = _inverseProjectionMatrix * glm::vec4(0, 0, 0, 1);
  _farDistance = fabs(farPoint.z / farPoint.w);

  _frustum.setInverseViewProjectionMatrix(_inverseProjectionMatrix);
}

glm::vec3 Camera::getDirection(
                              const glm::vec2& screenCoordinates) const noexcept
{
  glm::vec4 projectedPoint(screenCoordinates, 0, 1);
  glm::vec4 unprojectedPoint = (_inverseProjectionMatrix * projectedPoint);
  unprojectedPoint /= unprojectedPoint.w;
  unprojectedPoint = transformMatrix() * unprojectedPoint;
  return glm::normalize(glm::vec3(unprojectedPoint) - _eyePoint);
}
