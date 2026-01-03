#version 450

#include "textureViewer/viewer.inl"

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 arrayTexCoord = vec3(texCoord, 0);
  outColor = texture( sampler2DArray(colorTexture, textureSampler),
                      arrayTexCoord);
}