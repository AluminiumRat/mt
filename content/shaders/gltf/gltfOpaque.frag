#version 450

#include "gltf/gltfOpaque.inl"
#include "lib/lighting/brdf.inl"
#include "lib/lighting/ibl.inl"

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
    baseColor *= texture( sampler2D(baseColorTexture, commonLinearSampler),
                          inTexcoord0);
  #endif

  vec3 normal = normalize(inNormal);

  vec3 toViewer = commonData.cameraData.eyePoint - inWorldPosition;
  toViewer = normalize(toViewer);

  LitSurface surface = makeLitSurface(baseColor.rgb,
                                      materialBuffer.material.roughness,
                                      materialBuffer.material.metallic,
                                      normal,
                                      toViewer,
                                      commonData.environment.toSunDirection);

  vec3 radiance = getDirectLightRadiance(
                                  surface,
                                  commonData.environment.directLightIrradiance);

  radiance += getIBLRadiance( surface,
                              iblIrradianceMap,
                              iblSpecularMap,
                              commonData.environment.roughnessToLod);

  outColor = vec4(radiance, baseColor.a);
}