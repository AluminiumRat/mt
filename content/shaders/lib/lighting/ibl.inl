#ifndef IBL_INL
#define IBL_INL
//  Просчет image-based lighting

#include "lib/lighting/LitSurface.inl"
#include "lib/commonSet.inl"

vec3 getIBLRadiance(LitSurface surface,
                    textureCube iblIrradianceTexture,
                    textureCube iblSpecularTexture,
                    float roughnessToLodFactor)
{
  vec3 lutValue = textureLod( sampler2D(iblLut, iblLutSampler),
                              vec2(surface.normDotView, surface.roughness),
                              0).rgb;

  //  Ламбертова составляющая                              
  vec3 sampleIrradianceDir = vec3(surface.normal.x,
                                  surface.normal.y,
                                  -surface.normal.z);
  vec3 lambertRadiance = texture( samplerCube(iblIrradianceTexture,
                                              commonLinearSampler),
                                  sampleIrradianceDir).rgb;
  lambertRadiance *= surface.lambertColor;
  lambertRadiance *= lutValue.z;

  //  Спекуляр составляющая
  vec3 specularRadiance = surface.f0Reflection * lutValue.x + vec3(lutValue.y);
  float specularMapLod = surface.roughness * roughnessToLodFactor;
  vec3 reflectionDir = reflect(-surface.toViewer, surface.normal);
  reflectionDir.z = -reflectionDir.z;
  specularRadiance *= textureLod( samplerCube(iblSpecularTexture,
                                              commonLinearSampler),
                                  reflectionDir,
                                  specularMapLod).rgb;

  return lambertRadiance + specularRadiance;
}

#endif