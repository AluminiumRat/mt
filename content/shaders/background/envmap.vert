#version 450

#include "lib/commonSet.inl"

vec4 positions[4] = vec4[]( vec4(-1.0f, -1.0f, 0.0f, 1.0f),
                            vec4(-1.0f,  1.0f, 0.0f, 1.0f),
                            vec4( 1.0f, -1.0f, 0.0f, 1.0f),
                            vec4( 1.0f,  1.0f, 0.0f, 1.0f));

layout(location = 0) out vec3 outViewDirection;

void main()
{
  gl_Position = positions[gl_VertexIndex];

  vec4 worldPosition = commonData.cameraData.cullToWorldMatrix * gl_Position;
  worldPosition /= worldPosition.w;
  outViewDirection = worldPosition.xyz - commonData.cameraData.eyePoint;
  outViewDirection = normalize(outViewDirection);
}