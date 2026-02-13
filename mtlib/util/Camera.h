#pragma once

#include <glm/vec2.hpp>

#include <util/ViewFrustum.h>

namespace mt
{
  class Camera
  {
  public:
    //  Данные, предназначенные для записи в униформ буффер
    //  Предполагается, что эти данные попадут в CommonSet стадии или кадра
    //    и будут отсылаться на GPU небольшое количество раз (в идеале 1) за
    //    кадр. Поэтому сюда записывается как можно больше информации.
    struct ShaderData
    {
      alignas(16) glm::mat4 viewMatrix;
      alignas(16) glm::mat4 projectionMatrix;
      alignas(16) glm::mat4 viewProjectionMatrix;
      alignas(16) glm::vec3 eyePoint;
      alignas(16) glm::vec3 frontVector;
      alignas(16) glm::vec3 upVector;
      alignas(16) glm::vec3 rightVector;
    };

  public:
    ///  Коррекция матрицы проекции, связанная с разными экранными системами
    ///    координат в OpenGL и Vulkan. Взято с правками отсюда:
    ///    https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
    //  Компонент [2][2] изменен чтобы реализовать reversed-Z технику
    static constexpr glm::mat4 projectionCorrect = glm::mat4(
                                          glm::vec4( 1.0f,  0.0f,  0.0f, 0.0f),
                                          glm::vec4( 0.0f, -1.0f,  0.0f, 0.0f),
                                          glm::vec4( 0.0f,  0.0f, -0.5f, 0.0f),
                                          glm::vec4( 0.0f,  0.0f,  0.5f, 1.0f));

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

    /// Вью фрустум в view пространстве
    inline const ViewFrustum& frustum() const noexcept;

    /// Вью фрустум в мировом пространстве
    inline ViewFrustum worldFrustum() const noexcept;

    ///   Получить направление из eyePoint. Результат в той же системе
    ///     координат, в которой находится камера
    glm::vec3 getDirection(const glm::vec2& screenCoordinates) const noexcept;

    inline ShaderData makeShaderData() const noexcept;

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

  inline ViewFrustum Camera::worldFrustum() const noexcept
  {
    ViewFrustum translatedFrustum(_frustum);
    translatedFrustum.translate(_transformMatrix);
    return translatedFrustum;
  }

  inline Camera::ShaderData Camera::makeShaderData() const noexcept
  {
    ShaderData shaderData{};
    shaderData.viewMatrix = viewMatrix();
    shaderData.projectionMatrix = projectionMatrix();
    shaderData.viewProjectionMatrix =
                            shaderData.projectionMatrix * shaderData.viewMatrix;
    shaderData.eyePoint = eyePoint();
    shaderData.frontVector = frontVector();
    shaderData.upVector = upVector();
    shaderData.rightVector = rightVector();
    return shaderData;
  }
}