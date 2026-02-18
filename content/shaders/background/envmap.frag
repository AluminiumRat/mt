#version 450

#include "lib/commonSet.inl"

layout(location = 0) in vec3 inViewDirection;
layout(location = 0) out vec4 outColor;

void main()
{
  vec3 samplerDirection = vec3( inViewDirection.x,
                                inViewDirection.y,
                                -inViewDirection.z);
  outColor = textureLod(samplerCube(iblSpecularMap, commonLinearSampler),
                        samplerDirection,
                        0);
}