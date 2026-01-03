#version 450

#include "textureViewer/viewer.inl"

vec4 positions[4] = vec4[]( vec4(0.0f, 0.0f, 0.0f, 1.0f),
                            vec4(0.0f, 1.0f, 0.0f, 1.0f),
                            vec4(1.0f, 0.0f, 0.0f, 1.0f),
                            vec4(1.0f, 1.0f, 0.0f, 1.0f));

layout(location = 0) out vec2 outTexCoord;

void main()
{
  gl_Position = renderParams.modelMatrix * positions[gl_VertexIndex];
  gl_Position = renderParams.viewProjMatrix * gl_Position;
  outTexCoord = positions[gl_VertexIndex].xy;
}