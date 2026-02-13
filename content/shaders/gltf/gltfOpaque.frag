#version 450

#include "gltf/gltfOpaque.inl"
#include "lib/brdf.inl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inWorldPosition;
#if TEXCOORD_COUNT > 0
  layout(location = 2) in vec2 inTexcoord0;
#endif

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 baseColor = materialBuffer.material.baseColor;
  #if BASECOLORTEXTURE_ENABLED == 1 && TEXCOORD_COUNT > 0
    baseColor *= texture( sampler2D(baseColorTexture, linearSampler),
                          inTexcoord0);
  #endif

  vec3 normal = normalize(inNormal);

  vec3 toViewer = commonData.cameraData.eyePoint - inWorldPosition;
  toViewer = normalize(toViewer);

  vec3 halfVector = toViewer + commonData.lightData.toSunDirection;
  halfVector = normalize(halfVector);

  float normDotLight = dot(commonData.lightData.toSunDirection, normal);
  normDotLight = max(normDotLight, 0.001f);

  float normDotView = dot(toViewer, normal);
  normDotView = max(normDotView, 0.001f);

  float normDotHalf = dot(halfVector, normal);
  float viewDotHalf = dot(toViewer, halfVector);

  vec3 brdfValue = glt2BRDFFast(baseColor.rgb,
                                materialBuffer.material.roughness,
                                materialBuffer.material.metallic,
                                normDotLight,
                                normDotView,
                                normDotHalf,
                                viewDotHalf);


  vec3 irradiance = commonData.lightData.directLightIrradiance * normDotLight;

  outColor = vec4(irradiance * brdfValue,
                  baseColor.a);
}