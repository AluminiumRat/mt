#version 450

#include "examples/dds_load/shader.inl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = texture( sampler2D(colorTexture, samplerState),
                      texCoord);
}