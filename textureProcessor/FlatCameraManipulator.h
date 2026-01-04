#pragma once

#include <CameraManipulator.h>

namespace mt
{
  class Camera;
}

//  Манипулятор управляет камерой с ортогональной проекцией, направленной
//  вертикально вниз (вдоль оси Z в направлении -Z)
class FlatCameraManipulator : public CameraManipulator
{
public:
  explicit FlatCameraManipulator(mt::Camera& targetCamera);
  FlatCameraManipulator(const FlatCameraManipulator&) = delete;
  FlatCameraManipulator& operator = (const FlatCameraManipulator&) = delete;
  virtual ~FlatCameraManipulator() noexcept = default;

  virtual void update(ImVec2 areaPosition, ImVec2 areaSize) noexcept override;

  // xy координаты левого верхнего угла области видимости в координатах сцены
  inline glm::vec2 frustumOrigin() const noexcept;
  inline void setFrustumOrigin(glm::vec2 newOrigin) noexcept;

  // Размер области видимости в координатах сцены
  inline glm::vec2 frustumSize() const noexcept;
  inline void setFrustumSize(glm::vec2 newSize) noexcept;

  inline void setZRange(float minZ, float maxZ) noexcept;
  inline float minZ() const noexcept;
  inline float maxZ() const noexcept;

protected:
  virtual void onAreaResized(glm::ivec2 oldSize, glm::ivec2 newSize) override;
  virtual void onDragging(glm::ivec2 mouseDelta) override;

private:
  void _processMouseWheel() noexcept;
  void _aspectRatioCorrection() noexcept;
  void _updateCamera() noexcept;

private:
  mt::Camera& _targetCamera;
  glm::vec2 _frustumOrigin;   // xy координаты левого верхнего угла области
                              // видимости в координатах сцены
  glm::vec2 _frustumSize;     // Размер области видимости в координатах сцены
  float _minZ;
  float _maxZ;
};

inline glm::vec2 FlatCameraManipulator::frustumOrigin() const noexcept
{
  return _frustumOrigin;
}

inline void FlatCameraManipulator::setFrustumOrigin(glm::vec2 newOrigin) noexcept
{
  _frustumOrigin = newOrigin;
}

inline glm::vec2 FlatCameraManipulator::frustumSize() const noexcept
{
  return _frustumSize;
}

inline void FlatCameraManipulator::setFrustumSize(glm::vec2 newSize) noexcept
{
  _frustumSize = newSize;
}

inline void FlatCameraManipulator::setZRange(float minZ, float maxZ) noexcept
{
  _minZ = minZ;
  _maxZ = maxZ;
}

inline float FlatCameraManipulator::minZ() const noexcept
{
  return _minZ;
}

inline float FlatCameraManipulator::maxZ() const noexcept
{
  return _maxZ;
}
