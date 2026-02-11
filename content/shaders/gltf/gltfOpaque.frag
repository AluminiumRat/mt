#version 450

#include "gltf/gltfOpaque.inl"

layout(location = 0) in vec3 inNormal;
/*#if TEXCOORD_COUNT > 0
  layout(location = 1) in vec2 inTexcoord0;
#endif*/

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 normal = normalize(inNormal);
  float luminance = dot(commonData.lightData.toSunDirection, normal);
  luminance = clamp(luminance, 0.0f, 1.0f);
  luminance += 0.1f;

  outColor = vec4(luminance, luminance, luminance, 1.0);
}