#version 450

#include "examples/mesh/shader.inl"

layout(location = 0) out vec2 outTexCoord;

void main()
{
  vec4 position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
  gl_Position = transformData.positionMatrix[gl_InstanceIndex] * position;

  outTexCoord = vertices.data[gl_VertexIndex].position.xy + 0.5f;
}