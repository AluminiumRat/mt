#version 450

#include "lib/random.inl"

layout(location = 0) out vec2 outSample;

float radicalInverse2(uint value)
{
  float result = 0;
  float weight = 0.5f;
  while(value != 0)
  {
    result += weight * (value % 2);
    weight /= 2.0f;
    value /= 2;
  }
  return result;
}

float radicalInverse3(uint value)
{
  float result = 0;
  float weight = 1.0f / 3.0f;
  while(value != 0)
  {
    result += weight * (value % 3);
    weight /= 3.0f;
    value /= 3;
  }
  return result;
}

void main()
{
  vec2 quadSample = vec2( radicalInverse2(uint(gl_FragCoord.x)),
                          radicalInverse3(uint(gl_FragCoord.x)));
  outSample = concentricSampleDisk(quadSample);
}