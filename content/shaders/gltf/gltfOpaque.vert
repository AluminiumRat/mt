#version 450

#include "gltf/gltfOpaque.inl"

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outWorldPosition;
//  Тангент и бинормаль используются только для текстур нормалей
#if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
  layout(location = 2) out vec3 outTangent;
  layout(location = 3) out vec3 outBinormal;
#endif

// Текстурных координат может быть 0, 1 или 4
#if TEXCOORD_COUNT == 1
  layout(location = 4) out vec2 outTexcoord;
#endif
#if TEXCOORD_COUNT > 1
  layout(location = 4) out vec2 outTexcoord[4];
#endif

void main()
{
  #if INDICES_ENABLED == 1
    int vertexIndex = INDICES.data[gl_VertexIndex];
  #else
    int vertexIndex = gl_VertexIndex;
  #endif

  //  Сдвиг для буферов POSITION и NORMAL
  int vertexShift = vertexIndex * 3;

  vec4 position = vec4( POSITION.data[vertexShift],
                        POSITION.data[vertexShift + 1],
                        POSITION.data[vertexShift + 2],
                        1.0);
  position = transformData.positionMatrix[gl_InstanceIndex] * position;
  outWorldPosition = position.xyz;
  gl_Position = commonData.cameraData.viewProjectionMatrix * position;

  vec3 normal = vec3( NORMAL.data[vertexShift],
                      NORMAL.data[vertexShift + 1],
                      NORMAL.data[vertexShift + 2]);
  outNormal = transformData.bivecMatrix[gl_InstanceIndex] * normal;

  //  Тангент, если есть
  #if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
    outTangent = transformData.bivecMatrix[gl_InstanceIndex] *
                                                  TANGENT.data[vertexIndex].xyz;
    outBinormal = cross(outNormal, outTangent);
    outBinormal *= TANGENT.data[vertexIndex].w;
  #endif

  // Текстурные координаты
  #if TEXCOORD_COUNT == 1
    int texCoordShift = vertexIndex * 2;
    outTexcoord = vec2(TEXCOORD_0.data[texCoordShift],
                        TEXCOORD_0.data[texCoordShift + 1]);
  #endif
  #if TEXCOORD_COUNT > 1
    int texCoordShift = vertexIndex * 2;
    outTexcoord[0] = vec2(TEXCOORD_0.data[texCoordShift],
                          TEXCOORD_0.data[texCoordShift + 1]);
    outTexcoord[1] = vec2(TEXCOORD_1.data[texCoordShift],
                          TEXCOORD_1.data[texCoordShift + 1]);
    outTexcoord[2] = vec2(TEXCOORD_2.data[texCoordShift],
                          TEXCOORD_2.data[texCoordShift + 1]);
    outTexcoord[3] = vec2(TEXCOORD_3.data[texCoordShift],
                          TEXCOORD_3.data[texCoordShift + 1]);
  #endif
}