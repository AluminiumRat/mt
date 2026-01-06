#version 450

#include "textureViewer/viewer.inl"

layout(location = 0) in vec3 cubemapDirection;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 arrayTexCoord = vec4(cubemapDirection, renderParams.layer);
  arrayTexCoord.z = -arrayTexCoord.z;

  #if NEAREST_SAMPLER
    outColor = textureLod(samplerCubeArray(cubemapTexture, nearestSampler),
                          arrayTexCoord,
                          renderParams.mipIndex);
  #else
    outColor = textureLod(samplerCubeArray(cubemapTexture, linearSampler),
                          arrayTexCoord,
                          renderParams.mipIndex);
  #endif

  outColor *= renderParams.brightness;
}