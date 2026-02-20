#version 450

#include "gltf/gltfOpaque.inl"

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outWorldPosition;
//  Тангент и бинормаль используются только для текстур нормалей
#if NORMALTEXTURE_ENABLED == 1
  layout(location = 2) out vec3 outTangent;
  layout(location = 3) out vec3 outBinormal;
#endif
#if TEXCOORD_COUNT > 0
  layout(location = 4) out vec2 outTexcoord0;
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
  #if NORMALTEXTURE_ENABLED == 1
    outTangent = transformData.bivecMatrix[gl_InstanceIndex] *
                                                  TANGENT.data[vertexIndex].xyz;
    outBinormal = cross(outNormal, outTangent);
    outBinormal *= TANGENT.data[vertexIndex].w;
  #endif

  #if TEXCOORD_COUNT > 0
    int texCoordShift = vertexIndex * 2;
    outTexcoord0 = vec2(TEXCOORD_0.data[texCoordShift],
                        TEXCOORD_0.data[texCoordShift + 1]);
  #endif
}