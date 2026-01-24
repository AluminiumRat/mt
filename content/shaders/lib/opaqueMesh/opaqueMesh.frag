#version 450

#include "lib/opaqueMesh/opaqueMesh.inl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = texture( sampler2D(colorTexture, linearSampler),
                      texCoord);
}