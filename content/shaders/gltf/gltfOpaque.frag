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
    baseColor *= texture( sampler2D(baseColorTexture, commonLinearSampler),
                          inTexcoord0);
  #endif

  vec3 normal = normalize(inNormal);

  vec3 toViewer = commonData.cameraData.eyePoint - inWorldPosition;
  toViewer = normalize(toViewer);

  vec3 halfVector = toViewer + commonData.environment.toSunDirection;
  halfVector = normalize(halfVector);

  float normDotLight = dot(commonData.environment.toSunDirection, normal);
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

  vec3 irradiance = commonData.environment.directLightIrradiance * normDotLight;
  vec3 radiance = irradiance * brdfValue;

  //  IBL
  vec3 sampleIrradianceDir = vec3(normal.x, normal.y, -normal.z);
  vec3 lambertRradiance = texture(
                                samplerCube(iblIrradiance, commonLinearSampler),
                                sampleIrradianceDir).rgb;
  lambertRradiance *= baseColor.rgb;
  lambertRradiance *= (1.0f - materialBuffer.material.metallic);

  vec2 lutValue = textureLod(
                          sampler2D(iblLut, iblLutSampler),
                          vec2(normDotView, materialBuffer.material.roughness),
                          0).rg;
  vec3 f0Reflection = mix(vec3(DIELECTRIC_F0),
                          baseColor.rgb,
                          materialBuffer.material.metallic);
  vec3 specularRadiance = f0Reflection * lutValue.x + vec3(lutValue.y);

  vec3 specularMapDir = reflect(-toViewer,normal);
  specularMapDir.z = -specularMapDir.z;
  float specularMapLod = materialBuffer.material.roughness *
                                          commonData.environment.roughnessToLod;
  specularRadiance *= textureLod(
                              samplerCube(iblSpecularMap, commonLinearSampler),
                              specularMapDir,
                              specularMapLod).rgb;

  radiance += lambertRradiance;
  radiance += specularRadiance;

  outColor = vec4(radiance, baseColor.a);
}