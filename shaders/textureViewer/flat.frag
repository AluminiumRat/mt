#version 450

#include "textureViewer/viewer.inl"

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 arrayTexCoord = vec3(texCoord, renderParams.layer);

  #if NEAREST_SAMPLER
    outColor = textureLod(sampler2DArray(flatTexture, nearestSampler),
                          arrayTexCoord,
                          renderParams.mipIndex);
  #else
    outColor = textureLod(sampler2DArray(flatTexture, linearSampler),
                          arrayTexCoord,
                          renderParams.mipIndex);
  #endif

  outColor *= renderParams.brightness;
}