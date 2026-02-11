#version 450

#include "gltf/gltfOpaque.inl"

layout(location = 0) out vec3 outNormal;

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
  gl_Position = commonData.cameraData.viewProjectionMatrix * position;

  vec3 normal = vec3( NORMAL.data[vertexShift],
                      NORMAL.data[vertexShift + 1],
                      NORMAL.data[vertexShift + 2]);
  outNormal = transformData.bivecMatrix[gl_InstanceIndex] * normal;
}