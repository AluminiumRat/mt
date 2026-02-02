#version 450

#include "lib/color.inl"

layout (set = STATIC, binding = 0) uniform texture2D hdrTexture;

layout (set = STATIC, binding = 2) readonly buffer AvgLuminance
{
  float value;
} avgLuminance;

layout (set = STATIC, binding = 3) uniform texture2D bloomTexture;
layout (set = STATIC, binding = 4) uniform sampler linearSampler;

layout (set = STATIC, binding = 5) uniform Params
{
  float brightness;
  float maxWhite;
} params;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  vec3 sourceColor = texture( sampler2D(hdrTexture, linearSampler),
                              texCoord).rgb;
  sourceColor += texture( sampler2D(bloomTexture, linearSampler),
                                    texCoord).rgb;

  float sourceLuminance = colorToLuminance(sourceColor) + 0.001f;

  float maxWhite = params.maxWhite * avgLuminance.value;
  float resultLuminance = reinhardEx( sourceLuminance,
                                      maxWhite * maxWhite + 0.001f);

  vec3 resultColor = params.brightness *
                      resultLuminance / sourceLuminance * sourceColor;

  outColor = vec4(resultColor, 1.0f);
}