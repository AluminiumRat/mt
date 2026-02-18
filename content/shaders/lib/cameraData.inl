#ifndef CAMERA_DATA_INL
#define CAMERA_DATA_INL

//  Данные о состоянии камеры
//  Ответная c++ часть находится в util/Camera.h
struct CameraData
{
  mat4 viewMatrix;
  mat4 projectionMatrix;
  mat4 viewProjectionMatrix;
  mat4 viewToWorldMatix;
  mat4 invProjectionMatrix;
  mat4 cullToWorldMatrix;
  vec3 eyePoint;
  vec3 frontVector;
  vec3 upVector;
  vec3 rightVector;
};

#endif