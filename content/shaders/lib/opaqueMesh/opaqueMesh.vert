#version 450

#include "lib/opaqueMesh/opaqueMesh.inl"

layout(location = 0) out vec2 outTexCoord;

void main()
{
  vec4 position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
  position = transformData.positionMatrix[gl_InstanceIndex] * position;
  gl_Position = commonData.cameraData.viewProjectionMatrix * position;

  outTexCoord = vertices.data[gl_VertexIndex].position.xy + 0.5f;
}