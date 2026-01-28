#version 450

#include "lib/color.inl"

layout (set = STATIC, binding = 0) uniform texture2D hdrTexture;
layout (set = STATIC, binding = 1) uniform texture2D brightnessPyramid;
layout (set = STATIC, binding = 2) uniform texture2D avgColor;
layout (set = STATIC, binding = 3) uniform sampler linearSampler;

layout (set = STATIC, binding = 4) uniform Params
{
  float exposure;
  float maxWhiteSq;
} params;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  vec3 avgColor = texelFetch( sampler2D(avgColor, linearSampler),
                              ivec2(0,0),
                              0).rgb;
  float avgLuminance = colorToLuminance(avgColor);

  outColor = texture( sampler2D(hdrTexture, linearSampler),
                      texCoord);
  outColor.rgb = outColor.rgb * params.exposure / (avgLuminance + 0.001f);

  outColor.rgb = reinhard(outColor.rgb, params.maxWhiteSq);
}