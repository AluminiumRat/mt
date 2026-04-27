#version 450
//  Считаем зависимость угла рассеивания от шероховаточти
//  Нужно для контрэйсинга и выбора мип уровня в репроецированном HDR в расчете SSR
//  Кидаем лучи перпендикулярно поверхности и строим распределение отраженных
//    лучей по углу отражения. Сами лучи взвешиваем по косинусу угла к нормали и
//    по телесному углу сектора, в который попал луч.

#include "lib/lighting/brdf.inl"
#include "lib/random.inl"

layout (set = STATIC, binding = 1) uniform Params
{
  uint samplesCount;
} params;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

//  Телесный угол конуса с углом раствора angle
float solidAngle(float angle)
{
  return 2.0f * M_PI * (1.0f - cos(angle / 2.0f));
}

void main()
{
  float roughness = texCoord.x;
  float roughness_2 = roughness * roughness;
  float roughness_4 = roughness_2 * roughness_2;

  //  На сколько частей делим полусферу при подсчете распределения отраженных
  //  лучей.
  #define SECTORS_NUMBER 1000

  //  Телесный угол, который соответствует каждой группе в распределении
  //  отраженных лучей.
  //  Вычисляется как площадь кольца, которому соответствует диапазон углов
  //   на единичной сфере
  float squares[SECTORS_NUMBER];
  squares[0] = solidAngle(M_PI / SECTORS_NUMBER);
  for(uint i = 1; i < SECTORS_NUMBER; i++)
  {
    float angle = M_PI * (i + 1) / SECTORS_NUMBER;
    float fullSquare = solidAngle(angle);
    squares[i] = fullSquare - squares[i-1];
  }

  //  Распределение взвешанных лучей по углу отражения
  float weights[SECTORS_NUMBER];
  for(uint i = 0; i < SECTORS_NUMBER; i++) weights[i] = 0;

  float totalWeight = 0;
  for(uint i = 0; i < params.samplesCount; i++)
  {
    vec2 samplerValue = hammersley2d(i, params.samplesCount,0);
    vec3 microNormal = sampleGGX(samplerValue, roughness_4);
    vec3 reflectionDir = reflect(vec3(0,0,-1), microNormal);
    float angleCos = reflectionDir.z;
    if(angleCos > 0)
    {
      float angle = acos(angleCos);
      int counterIndex = int(angle / (M_PI / 2.0f) * SECTORS_NUMBER);
      counterIndex = min(counterIndex, SECTORS_NUMBER - 1);
      float weight = angleCos / squares[counterIndex];
      weights[counterIndex] += weight;
      totalWeight += weight;
    }
  }

  //  Ищем угол, внутри которого лежит не менее 90% веса лучей
  float weight = 0;
  for(uint i = 0; i < SECTORS_NUMBER; i++)
  {
    weight += weights[i];
    if(weight > 0.9f * totalWeight)
    {
      float angle = M_PI / 2.0f / SECTORS_NUMBER * i;
      outColor = vec4(tan(angle),0,0,1);
      return;
    }
  }

  outColor = vec4(0,0,0,1);
}