#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <CameraManipulator.h>

namespace mt
{
  class Camera;
}

//  Управляет камерой, которая двигается по сфере вокруг фиксированной точки
//  (центра)
class OrbitalCameraManipulator : public CameraManipulator
{
public:
  explicit OrbitalCameraManipulator(mt::Camera& targetCamera);
  OrbitalCameraManipulator(const OrbitalCameraManipulator&) = delete;
  OrbitalCameraManipulator& operator = (
                                    const OrbitalCameraManipulator&) = delete;
  virtual ~OrbitalCameraManipulator() noexcept = default;

  virtual void update(ImVec2 areaPosition, ImVec2 areaSize) noexcept override;

  //  Точка, вокруг которой двигается камера
  inline const glm::vec3& centerPosition() const noexcept;
  inline void setCenterPosition(const glm::vec3& newValue) noexcept;

  //  Расстояние от центра вращения до камеры
  inline float distance() const noexcept;
  inline void setDistance(float newValue) noexcept;

  inline float yawAngle() const noexcept;
  inline void setYawAngle(float newValue) noexcept;

  inline float pitchAngle() const noexcept;
  inline void setPitchAngle(float newValue) noexcept;

  //  Чувствительность мышки
  inline float rotateSensitivity() const noexcept;
  inline void setRotateSensitivity(float newValue) noexcept;

  //  Настройки проекции для камеры
  inline float fowY() const noexcept;
  inline float nearPlane() const noexcept;
  inline float farPlane() const noexcept;
  inline void setProjectionParams(float fowY,
                                  float nearPlane,
                                  float farPlane) noexcept;

protected:
  virtual void onDragging(glm::ivec2 mouseDelta) override;

private:
  void _processMouseWheel() noexcept;
  void _updateCameraPosition() noexcept;
  void _updateProjectionMatrix(ImVec2 areaSize) noexcept;

private:
  mt::Camera& _targetCamera;

  glm::vec3 _centerPosition;
  float _distance;
  float _yawAngle;
  float _pitchAngle;

  float _fowY;
  float _nearPlane;
  float _farPlane;

  float _rotateSensitivity;
};

inline const glm::vec3&
                      OrbitalCameraManipulator::centerPosition() const noexcept
{
  return _centerPosition;
}

inline void OrbitalCameraManipulator::setCenterPosition(
                                          const glm::vec3& newValue) noexcept
{
  _centerPosition = newValue;
}

inline float OrbitalCameraManipulator::distance() const noexcept
{
  return _distance;
}

inline void OrbitalCameraManipulator::setDistance(float newValue) noexcept
{
  _distance = newValue;
}

inline float OrbitalCameraManipulator::yawAngle() const noexcept
{
  return _yawAngle;
}

inline void OrbitalCameraManipulator::setYawAngle(float newValue) noexcept
{
  _yawAngle = newValue;
}

inline float OrbitalCameraManipulator::pitchAngle() const noexcept
{
  return _pitchAngle;
}

inline void OrbitalCameraManipulator::setPitchAngle(float newValue) noexcept
{
  _pitchAngle = newValue;
}

inline float OrbitalCameraManipulator::rotateSensitivity() const noexcept
{
  return _rotateSensitivity;
}

inline void OrbitalCameraManipulator::setRotateSensitivity(float newValue) noexcept
{
  _rotateSensitivity = newValue;
}

inline float OrbitalCameraManipulator::fowY() const noexcept
{
  return _fowY;
}

inline float OrbitalCameraManipulator::nearPlane() const noexcept
{
  return _nearPlane;
}

inline float OrbitalCameraManipulator::farPlane() const noexcept
{
  return _farPlane;
}

inline void OrbitalCameraManipulator::setProjectionParams(
                                                      float fowY,
                                                      float nearPlane,
                                                      float farPlane) noexcept
{
  _fowY = fowY;
  _nearPlane = nearPlane;
  _farPlane = farPlane;
}
