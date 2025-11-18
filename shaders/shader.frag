#version 450

#include "shader.inl"

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  #if selector2 == 0
    outColor = vec4(vertexColor, 1.0);
  #else
    outColor = vec4(1, selector1 / 2.0, selector1 % 2, 1);
  #endif

  outColor *= texture(sampler2D(colorTexture1, samplerState),
                      texCoord);
  outColor *= texture(sampler2D(colorTexture2, samplerState),
                      texCoord);
  //outColor *= colorData.color;
  //outColor = colorData.color * vec4(vertexColor, 1.0);
}