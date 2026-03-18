#version 460
#extension GL_EXT_ray_query : enable

#include "lib/commonSet.inl"

layout (set = STATIC, binding = 0) uniform sampler nearestSampler;
layout (set = STATIC, binding = 1) uniform accelerationStructureEXT tlas;

layout(location = 0) in vec2 texCoord;
layout(location = 2) in vec3 inPosRestoreVec;

layout(location = 0) out float outShadowFactor;

void main()
{
  float depth = texture(sampler2D(linearDepthHalfBuffer, nearestSampler),
                        texCoord).r;
  if(depth != 10000.0f)
  {
    vec3 worldPosition = commonData.cameraData.eyePoint +
                                                        inPosRestoreVec * depth;

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery,
                          tlas,
                          gl_RayFlagsTerminateOnFirstHitEXT,
                          0xFF,
                          worldPosition,
                          0.01f,
                          commonData.environment.toSunDirection,
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