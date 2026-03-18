#version 450
//  Вершинный шейдер для полноэкранного квадрата.
//  Дополнительно вычисляет данные, связанные с текущей камерой

#include "lib/commonSet.inl"

vec4 positions[4] = vec4[]( vec4(-1.0f, -1.0f, 0.0f, 1.0f),
                            vec4(-1.0f,  1.0f, 0.0f, 1.0f),
                            vec4( 1.0f, -1.0f, 0.0f, 1.0f),
                            vec4( 1.0f,  1.0f, 0.0f, 1.0f));

layout(location = 0) out vec2 outTexCoord;
//  Нормализованное направление "из камеры" в мировых координатах
layout(location = 1) out vec3 outViewDirection;
//  Вектор, специально предназначенный для вычисления мировых координат
//  по линейному буферу глубины. Представляет из себя напрвление "из камеры",
//  но с нормализованной во вью пространстве координатой z
layout(location = 2) out vec3 outPosRestoreVec;

void main()
{
  gl_Position = positions[gl_VertexIndex];
  outTexCoord = gl_Position.xy * 0.5f + 0.5f;

  vec4 viewPosition = commonData.cameraData.invProjectionMatrix * gl_Position;
  viewPosition /= viewPosition.w;
  //  Во вью пространстве камера направлена в -z, поэтому делим так же на -z
  viewPosition.xyz /= -viewPosition.z;
  vec4 worldPosition = commonData.cameraData.viewToWorldMatix * viewPosition;
  outPosRestoreVec = worldPosition.xyz - commonData.cameraData.eyePoint;

  outViewDirection = normalize(outPosRestoreVec);
}