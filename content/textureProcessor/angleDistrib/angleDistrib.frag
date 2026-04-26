#version 450
//  Считаем зависимость угла рассеивания от шероховаточти

#include "lib/lighting/brdf.inl"
#include "lib/random.inl"

layout (set = STATIC, binding = 1) uniform Params
{
  uint samplesCount;
} params;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  float roughness = texCoord.x;
  float roughness_2 = roughness * roughness;
  float roughness_4 = roughness_2 * roughness_2;

  //  Распределение сэмплов по углу
  #define COUNTERS_NUMBER 1000
  float conters[COUNTERS_NUMBER];
  for(uint i = 0; i < COUNTERS_NUMBER; i++) conters[i] = 0;

  float totalCounter = 0;
  for(uint i = 0; i < params.samplesCount; i++)
  {
    vec2 samplerValue = hammersley2d(i, params.samplesCount,0);
    vec3 microNormal = sampleGGX(samplerValue, roughness_4);
    vec3 reflectionDir = reflect(vec3(0,0,-1), microNormal);
    if(reflectionDir.z > 0)
    {
      float tan = length(reflectionDir.xy) / reflectionDir.z;
      float angle = atan(tan);
      int counterIndex = int(angle / (M_PI / 2.0f) * COUNTERS_NUMBER);
      counterIndex = min(counterIndex, COUNTERS_NUMBER - 1);
      conters[counterIndex] += reflectionDir.z;
      totalCounter += reflectionDir.z;
    }
  }

  //  Ищем угол, внутри которого лежит не менее 80% сэмплов
  float countersSumm = 0;
  for(uint i = 0; i < COUNTERS_NUMBER; i++)
  {
    countersSumm += conters[i];
    if(countersSumm > 0.8f * totalCounter)
    {
      float angle = M_PI / 2.0f / COUNTERS_NUMBER * i;
      outColor = vec4(angle,0,0,1);
      return;
    }
  }

  outColor = vec4(M_PI / 2.0f,0,0,1);
}