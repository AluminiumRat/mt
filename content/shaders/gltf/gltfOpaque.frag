#version 450

#include "gltf/gltfFragCommon.inl"
#include "lib/lighting/brdf.inl"
#include "lib/lighting/ibl.inl"
#include "lib/octahedronEncoding.inl"

layout(location = 0) out vec4 outColor;

float getShadowFactor(vec3 normal)
{
  //  Считываем сразу 4 значения из карты теней
  vec2 gatherCoords = inSSCoords;// - commonData.halfFrameExtent.zw * 0.5f;
  vec4 shadowValues = textureGather(sampler2D( shadowBuffer, commonLinearSampler),
                                    gatherCoords,
                                    0);
  //  Взвешиваем их по расстояниям до центров текселей буфера теней
  vec2 shifts = fract(gatherCoords * commonData.halfFrameExtent.xy);
  vec4 weights = vec4(shifts.x * (1.0 - shifts.y),
                      (1.0f  - shifts.x) * (1.0 - shifts.y),
                      (1.0f  - shifts.x) * shifts.y,
                      shifts.x * shifts.y);

  //  Взвешиваем по нормалям
  vec4 normalValues1 = textureGather(sampler2D(normalHalfBuffer, commonLinearSampler),
                                     gatherCoords,
                                     0);
  vec4 normalValues2 = textureGather(sampler2D(normalHalfBuffer, commonLinearSampler),
                                     gatherCoords,
                                     1);
  vec4 normalWeights = vec4(
          dot(octahedronDecode(vec2(normalValues1.x, normalValues2.x)), normal),
          dot(octahedronDecode(vec2(normalValues1.y, normalValues2.y)), normal),
          dot(octahedronDecode(vec2(normalValues1.z, normalValues2.z)), normal),
          dot(octahedronDecode(vec2(normalValues1.w, normalValues2.w)), normal));
  normalWeights = clamp(normalWeights, 0.001f, 1.0f);
  weights *= normalWeights;

  //  Взвешивание по дистанции
  //  Размер пикселя в метрах в точке, где накладываем тень.
  float pixelSize = commonData.cameraData.fovY * inWorldPosition.w *
                                                commonData.halfFrameExtent.w;
  float depthTreshold = 5.0f * pixelSize;
  vec4 depths = textureGather(sampler2D(linearDepthHalfBuffer,
                                        commonLinearSampler),
                              gatherCoords,
                              0);
  vec4 depthDeltas = abs(vec4(inWorldPosition.w) - depths);
  weights *= clamp(vec4(depthTreshold) - depthDeltas, 0.001f, depthTreshold);

  //  Возвращаем средневзвешанное значение
  return dot(shadowValues, weights) /
          max((weights.x + weights.y + weights.z + weights.w), 0.001f);
}

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

  vec3 toViewer = commonData.cameraData.eyePoint - inWorldPosition.xyz;
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

  float shadowFactor = getShadowFactor(normal);
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