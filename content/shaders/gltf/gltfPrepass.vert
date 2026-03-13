#version 450

#include "gltf/gltfOpaque.inl"

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
}