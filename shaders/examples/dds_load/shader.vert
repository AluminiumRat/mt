#version 450

#include "examples/dds_load/shader.inl"

layout(location = 0) out vec2 outTexCoord;

void main()
{
  gl_Position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
  outTexCoord = vertices.data[gl_VertexIndex].position.xy + 0.5f;
}