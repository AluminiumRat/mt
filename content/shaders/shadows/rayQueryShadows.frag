#version 460
#extension GL_EXT_ray_query : enable

#include "lib/commonSet.inl"

layout (set = STATIC, binding = 0) uniform sampler nearestSampler;
layout (set = STATIC, binding = 1) uniform accelerationStructureEXT tlas;
layout (set = STATIC, binding = 2) uniform texture2D noiseTexture;
layout (set = STATIC, binding = 3) uniform texture1D samplerTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 2) in vec3 inPosRestoreVec;

layout(location = 0) out float outShadowFactor;

void main()
{
  float depth = texture(sampler2D(linearDepthHalfBuffer, nearestSampler),
                        texCoord).r;
  if(depth != 10000.0f)
  {
    float random = texelFetch( sampler2D(noiseTexture, nearestSampler),
                                ivec2(int(gl_FragCoord.x) % 32,
                                      int(gl_FragCoord.y) % 32),
                                0).r;
    int sampleIndex = int(random * 255.f);
    vec2 samplerValue = texelFetch( sampler1D(samplerTexture, nearestSampler),
                                    sampleIndex,
                                    0).rg;
    samplerValue *= commonData.environment.sunAngleSize / 2.0f;
    vec3 rayDirection =
              commonData.environment.toSunDirection +
                commonData.environment.sunLatitudeDirection * samplerValue.x +
                commonData.environment.sunLongitudeDirection * samplerValue.y;

    vec3 worldPosition = commonData.cameraData.eyePoint +
                                                        inPosRestoreVec * depth;

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery,
                          tlas,
                          gl_RayFlagsTerminateOnFirstHitEXT,
                          0xFF,
                          worldPosition,
                          0.01f,
                          rayDirection,
                          1000.0f);
    rayQueryProceedEXT(rayQuery);
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) ==
                                    gl_RayQueryCommittedIntersectionTriangleEXT)
    {
      outShadowFactor = 0.0f;
    }
    else outShadowFactor = 1.0f;
  }
  else outShadowFactor = 1.0f;
}