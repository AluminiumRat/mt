#version 450

#include "textureViewer/square.inl"

vec2 positions[4] = vec2[]( vec2(-1.0f, -1.0f),
                            vec2(-1.0f,  1.0f),
                            vec2( 1.0f, -1.0f),
                            vec2( 1.0f,  1.0f));

layout(location = 0) out vec2 outTexCoord;

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
  outTexCoord = positions[gl_VertexIndex].xy * 0.5f + 0.5f;
}