#version 460
#extension GL_EXT_ray_query : enable

#include "lib/commonSet.inl"

layout (set = STATIC, binding = 0) uniform texture2D depthMap;
layout (set = STATIC, binding = 1) uniform sampler nearestSampler;
layout (set = STATIC, binding = 2) uniform accelerationStructureEXT tlas;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  float depth = texture(sampler2D(depthMap, nearestSampler), texCoord).r;
  if(depth != 0)
  {
    vec4 clipSpacePosition = vec4(texCoord * 2.0f - 1.0f,
                                  depth,
                                  1.0f);
    vec4 worldPosition =
                    commonData.cameraData.cullToWorldMatrix * clipSpacePosition;
    worldPosition /= worldPosition.w;

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery,
                          tlas,
                          gl_RayFlagsTerminateOnFirstHitEXT,
                          0xFF,
                          worldPosition.xyz,
                          0.01f,
                          commonData.environment.toSunDirection,
                          1000.0f);
    rayQueryProceedEXT(rayQuery);
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) ==
                                    gl_RayQueryCommittedIntersectionTriangleEXT)
    {
      outColor = vec4(0.0f);
    }
    else outColor = vec4(1.0f);
  }
  else outColor = vec4(1.0f);
}