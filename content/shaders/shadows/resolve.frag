#version 450

layout (set = STATIC, binding = 0) uniform texture2D depthMap;
layout (set = STATIC, binding = 1) uniform sampler nearestSampler;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  float depth = 1.0f - texture( sampler2D(depthMap, nearestSampler),
                                texCoord).r;
  outColor = vec4(depth);
}