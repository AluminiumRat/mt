#ifndef CAMERA_DATA_INL
#define CAMERA_DATA_INL

//  Данные о состоянии камеры
//  Ответная c++ часть находится в util/Camera.h
struct CameraData
{
  //  Матрицы преобразования, связанные с камерой
  mat4 viewMatrix;
  mat4 projectionMatrix;
  mat4 viewProjectionMatrix;
  mat4 viewToWorldMatix;
  mat4 invProjectionMatrix;
  mat4 cullToWorldMatrix;

  //  Условное место, откуда происходит наблюдение
  //  Для перспективной проекции - центр проекции
  vec3 eyePoint;

  //  Векторы, задающие ориентацию камеры в мировых координатах
  vec3 frontVector;
  vec3 upVector;
  vec3 rightVector;
};

#endif