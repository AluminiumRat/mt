#version 450

layout (set = STATIC) uniform texture1D samplesTexture;
layout (set = STATIC) uniform sampler textureSampler;

layout (set = STATIC) uniform Params
{
  uint startSample;
  uint samplesCount;
} params;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
  vec2 screenPos = texCoord * 2.0f - 1.0f;
  float value = 0.0f;

  for(uint i = params.startSample; i < params.startSample + params.samplesCount; i++)
  {
    vec2 sampleValue = texelFetch(sampler1D(samplesTexture, textureSampler),
                                  int(i),
                                  0).xy;
    //value = sampleValue.y / 2.0f + 0.5f;
    if(length(sampleValue - screenPos) < 0.01f) value = 1;
  }

  outColor = vec4(value);
}