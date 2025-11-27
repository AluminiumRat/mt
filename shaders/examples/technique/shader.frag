#version 450

#include "examples/technique/shader.inl"

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  // Сеоектор - это просто дефайн, который выставляется компилятором
  #if colorSelector == 0
    outColor = vec4(vertexColor, 1.0);
  #else
    outColor = texture( sampler2D(colorTexture, samplerState),
                        texCoord);
  #endif

  outColor *= colorData.color;
}