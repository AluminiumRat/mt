#pragma once

#include <glm/vec2.hpp>

#include <util/ViewFrustum.h>

namespace mt
{
  class Camera
  {
  public:
    // Correction of projection matrix was getted from
    // https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
    // Component [2][2] was changed to implement reversed-Z technique
    static constexpr glm::mat4 projectionCorrect = glm::mat4(
                                              glm::vec4(1.f,  0.f,  0.f, 0.f),
                                              glm::vec4(0.f, -1.f,  0.f, 0.f),
                                              glm::vec4(0.f,  0.f, -.5f, 0.f),
                                              glm::vec4(0.f,  0.f,  .5f, 1.f));

  public:
    Camera() noexcept;
    Camera(const Camera&) = delete;
    Camera& operator = (const Camera&) = delete;
    virtual ~Camera() noexcept = default;

    inline const glm::mat4& transformMatrix() const noexcept;
    virtual void setTransformMatrix(const glm::mat4& newValue) noexcept;

    inline const glm::vec3& eyePoint() const noexcept;
    inline const glm::vec3& frontVector() const noexcept;
    inline const glm::vec3& upVector() const noexcept;
    inline const glm::vec3& rightVector() const noexcept;
    inline const glm::mat4 viewMatrix() const noexcept;

    void setProjectionMatrix(glm::mat4 newValue) noexcept;
    void setPerspectiveProjection(float fovY,
                                  float aspect,
                                  float zNear,
                                  float zFar) noexcept;
    void setOrthoProjection(float left,
                            float right,
                            float bottom,
                            float top,
                            float zNear,
                            float zFar) noexcept;
    inline const glm::mat4 projectionMatrix() const noexcept;
    inline const glm::mat4 inverseProjectionMatrix() const noexcept;
    inline float nearDistance() const noexcept;
    inline float farDistance() const noexcept;

    /// Frustum in view space
    inline const ViewFrustum& frustum() const noexcept;

    /// Returns direction from eyePoint
    glm::vec3 getDirection(const glm::vec2& screenCoordinates) const noexcept;

  private:
    void _updateFromPositionMatrix() noexcept;
    void _updateFromProjectionMatrix() noexcept;

  private:
    glm::mat4 _viewMatrix;
    glm::vec3 _eyePoint;
    glm::vec3 _frontVector;
    glm::vec3 _upVector;
    glm::vec3 _rightVector;

    glm::mat4 _transformMatrix;
    glm::mat4 _projectionMatrix;
    glm::mat4 _inverseProjectionMatrix;

    ViewFrustum _frustum;

    float _nearDistance;
    float _farDistance;
  };

  inline const glm::mat4& Camera::transformMatrix() const noexcept
  {
    return _transformMatrix;
  }

  inline const glm::vec3& Camera::eyePoint() const noexcept
  {
    return _eyePoint;
  }

  inline const glm::vec3& Camera::frontVector() const noexcept
  {
    return _frontVector;
  }

  inline const glm::vec3& Camera::upVector() const noexcept
  {
    return _upVector;
  }

  inline const glm::vec3& Camera::rightVector() const noexcept
  {
    return _rightVector;
  }

  inline const glm::mat4 Camera::viewMatrix() const noexcept
  {
    return _viewMatrix;
  }

  inline const glm::mat4 Camera::projectionMatrix() const noexcept
  {
    return _projectionMatrix;
  }

  inline const glm::mat4 Camera::inverseProjectionMatrix() const noexcept
  {
    return _inverseProjectionMatrix;
  }

  inline float Camera::nearDistance() const noexcept
  {
    return _nearDistance;
  }

  inline float Camera::farDistance() const noexcept
  {
    return _farDistance;
  }

  inline const ViewFrustum& Camera::frustum() const noexcept
  {
    return _frustum;
  }
}