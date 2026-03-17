#version 450

#include "gltf/gltfFragCommon.inl"
#include "lib/lighting/brdf.inl"
#include "lib/lighting/ibl.inl"

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 baseColor = materialBuffer.material.baseColor;
  #if TEXCOORD_COUNT != 0
    baseColor *= texture(
                        sampler2D(baseColorTexture, commonLinearSampler),
                        getTexCoord(materialBuffer.material.baseColorTexCoord));
  #endif

  float roughness = materialBuffer.material.roughness;
  float metallic = materialBuffer.material.metallic;
  #if TEXCOORD_COUNT > 0
    vec4 mRFromTexture = texture(
                sampler2D(metallicRougghnessTexture, commonLinearSampler),
                getTexCoord(materialBuffer.material.metallicRoughnessTexCoord));
    roughness *= mRFromTexture.g;
    metallic *= mRFromTexture.b;
  #endif

  vec3 normal = getNormal();

  #if TEXCOORD_COUNT > 0
    float occlusion = texture(
                      sampler2D(occlusionTexture,commonLinearSampler),
                      getTexCoord(materialBuffer.material.occlusionTexCoord)).r;
    occlusion *= materialBuffer.material.occlusionTextureStrength;
  #else
    float occlusion = 1.0f;
  #endif

  vec3 toViewer = commonData.cameraData.eyePoint - inWorldPosition;
  toViewer = normalize(toViewer);

  ObservedSurface observedSurface = makeObservedSurface(baseColor.rgb,
                                                        roughness,
                                                        metallic,
                                                        occlusion,
                                                        normal,
                                                        toViewer);
  LitSurface litSurface = makeLitSurface(
                                        observedSurface,
                                        commonData.environment.toSunDirection);

  float shadowFactor = texture( sampler2D( shadowBuffer, commonLinearSampler),
                                inSSCoords).r;
  vec3 radiance = getDirectLightRadiance(
                                  observedSurface,
                                  litSurface,
                                  commonData.environment.directLightIrradiance,
                                  shadowFactor);

  radiance += getIBLRadiance( observedSurface,
                              iblIrradianceMap,
                              iblSpecularMap,
                              commonData.environment.roughnessToLod);

  #if EMISSIVETEXTURE_ENABLED  && TEXCOORD_COUNT > 0
    vec3 emission = texture(
                    sampler2D(emissiveTexture, commonLinearSampler),
                    getTexCoord(materialBuffer.material.emissiveTexCoord)).rgb;

    emission *= materialBuffer.material.emission;
    radiance += emission;
  #endif

  outColor = vec4(radiance, baseColor.a);
}