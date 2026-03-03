#version 450

#include "gltf/gltfOpaque.inl"
#include "lib/lighting/brdf.inl"
#include "lib/lighting/ibl.inl"
#include "lib/color.inl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inWorldPosition;
//  Тангент и бинормаль используются только для текстур нормалей
#if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
  layout(location = 2) in vec3 inTangent;
  layout(location = 3) in vec3 inBinormal;
#endif

#if TEXCOORD_COUNT == 1
  layout(location = 4) in vec2 inTexCoord;
#endif
#if TEXCOORD_COUNT > 1
  layout(location = 4) in vec2 inTexCoord[4];
#endif

layout(location = 0) out vec4 outColor;

//  Получить текстурные координаты по их индексу
#if TEXCOORD_COUNT > 0
vec2 getTexCoord(int index)
{
  #if TEXCOORD_COUNT == 1
    return inTexCoord;
  #else
    return inTexCoord[index];
  #endif
}
#endif

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

  vec3 normal = normalize(inNormal);
  #if NORMALTEXTURE_MODE != NORMALTEXTURE_OFF && TEXCOORD_COUNT > 0
    vec3 textureNormal = texture(
                      sampler2D(normalTexture, commonLinearSampler),
                      getTexCoord(materialBuffer.material.normalTexCoord)).rgb;
    //  Считанное значение прошло в сэмплере через sRGB->linear преобразование.
    //  Но исходные значения и так были в линейном пространстве, поэтому
    //  переводим обратно
    textureNormal = linearToSrgbFast(textureNormal);
    textureNormal = textureNormal * 2.0f - 1.0f;

    #if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
      vec3 tangent = normalize(inTangent);
      vec3 binormal = normalize(inBinormal);
    #else
      //  NORMAL_TEXTURE_FRAGMENT_TANGENT
      vec3 tangent = normalize( dFdx(inWorldPosition) * dFdx(getTexCoord(0).x) +
                                dFdy(inWorldPosition) * dFdy(getTexCoord(0).x));
      vec3 binormal = -normalize(cross(normal, tangent));
    #endif
    mat3 tbn = mat3(tangent, binormal, normal);

    normal = mix( normal,
                  tbn * textureNormal,
                  materialBuffer.material.normalTextureScale);
    normal = normalize(normal);
  #endif

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

  //  Теней пока нет, поэтому вместо shadowFactor передаем ambientOcclusion
  //  Костыль, но позволяет сделать немного покарасивее
  vec3 radiance = getDirectLightRadiance(
                                  observedSurface,
                                  litSurface,
                                  commonData.environment.directLightIrradiance,
                                  observedSurface.ambientOcclusion);

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