#version 450

layout (set = STATIC, binding = 0) uniform texture2D hdrTexture;
layout (set = STATIC, binding = 1) uniform texture2D brightnessPyramid;
layout (set = STATIC, binding = 2) uniform texture2D avgColor;
layout (set = STATIC, binding = 3) uniform sampler linearSampler;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  vec3 avgColor = texelFetch( sampler2D(avgColor, linearSampler),
                              ivec2(0,0),
                              0).rgb;
  float avgLuminance = dot(avgColor, vec3(0.21f, 0.72f, 0.7f));

  outColor = texture( sampler2D(hdrTexture, linearSampler),
                      texCoord);
  outColor.rgb = outColor.rgb / avgLuminance;
}