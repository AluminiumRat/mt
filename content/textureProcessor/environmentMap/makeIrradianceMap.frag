#version 450
//  Построение предзапеченой irradiance cubemap по cubemap-е окружения

#include "lib/cubemap.inl"
#include "lib/random.inl"

layout (set = STATIC, binding = 0) uniform IntrinsicData
{
  uint mipLevel;
  uint arrayIndex;
} intrinsic;

layout (set = STATIC, binding = 1) uniform Params
{
  uint samplesPerTexel;
} params;

layout (set = STATIC, binding = 2) uniform textureCube environmentMap;
layout (set = STATIC, binding = 3) uniform sampler colorSampler;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  //  Нормаль, для которой вычисляется освещенность
  vec3 normal = cubemapDirection(texCoord, intrinsic.arrayIndex);

  //  Просто два вектора, образующих с нормалью ортогональный базис
  vec3 binormal = abs(normal.z) > 0.9f ? vec3(1, 0, 0) : vec3(0, 1, 1);
  vec3 tangent = normalize(cross(normal, binormal));
  binormal = normalize(cross(normal, tangent));

  //  Матрица для перевода направления сэмплирования в мировые
  //  координаты
  mat3 tbn = mat3(tangent, binormal, normal);

  //  Вычисляем, какой лод исходной текстуры будем сэмплить
  //  Определяем по количеству текселй нулевого лода, которые должен
  //  накрыть одна выборка из текстуры
  int cubemapSize = textureSize(samplerCube(environmentMap, colorSampler), 0).x;
  int texelsPerHemisphere = 3 * cubemapSize * cubemapSize;
  float texelsPerSample = float(texelsPerHemisphere) / params.samplesPerTexel;
  float cubemapLod = log2(texelsPerSample) / 2.f + 2.f;

  //  Интегрируем яркость по полусфере, чтобы получить освещенность
  vec3 color = vec3(0.0);
  for(int i = 0; i < params.samplesPerTexel; i++)
  {
    float seed = random(texCoord);
    vec2 samplerValue = hammersley2d(i,
                                     params.samplesPerTexel,
                                     uint(seed * 10 * params.samplesPerTexel));
    vec3 sampleDirection = sampleHemisphere(samplerValue);
    
    float cosTheta = sampleDirection.z;

    sampleDirection = tbn * sampleDirection;
    sampleDirection.z = -sampleDirection.z;

    color += textureLod(samplerCube(environmentMap, colorSampler),
                        sampleDirection,
                        cubemapLod).rgb * cosTheta;
  }

  outColor = vec4(2.0f * color / float(params.samplesPerTexel), 1.0);  
}