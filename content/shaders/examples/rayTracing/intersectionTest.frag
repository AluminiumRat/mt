#version 460
#extension GL_EXT_ray_query : enable

layout (set = STATIC, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(texCoord, 1.0f, 1.0f);

  vec2 rayStart = texCoord * 2.f - 1.f;

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery,
                        tlas,
                        gl_RayFlagsTerminateOnFirstHitEXT,
                        0xFF,
                        vec3(rayStart, 20.0f),
                        0.01f,
                        vec3(0.0f, 0.0f, -1.0f),
                        1000.0f);
	rayQueryProceedEXT(rayQuery);
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
  {
		outColor.rgb *= 0.0f;
	}
}