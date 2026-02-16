#version 450
//  Построение prefiltered environment map по cubemap-е окружения
//  https://learnopengl.com/PBR/IBL/Specular-IBL

#include "lib/brdf.inl"
#include "lib/cubemap.inl"
#include "lib/random.inl"

layout (set = STATIC, binding = 0) uniform IntrinsicData
{
  uint mipLevel;
  uint mipLevelCount;
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
  //  Макронормаль, вокруг которой фильтруется изображение
  vec3 normal = cubemapDirection(texCoord, intrinsic.arrayIndex);

  //  Просто два вектора, образующих с нормалью ортогональный базис
  vec3 binormal = abs(normal.z) > 0.9f ? vec3(1, 0, 0) : vec3(0, 1, 1);
  vec3 tangent = normalize(cross(normal, binormal));
  binormal = normalize(cross(normal, tangent));

  //  Матрица для перевода направления сэмплирования в мировые
  //  координаты
  mat3 tbn = mat3(tangent, binormal, normal);

  float roughness = float(intrinsic.mipLevel) / (intrinsic.mipLevelCount - 1);
  float roughness_2 = roughness * roughness;
  float roughness_4 = roughness_2 * roughness_2;

  //  Вычисляем телесный угол одного пикселя исходной карты окружения
  //  Позже понадобится нам для определения лода, из которого нужно считывать
  //  окружение
  float envMapSize = float( textureSize(samplerCube(environmentMap,
                                                    colorSampler),
                                        0).x);
  float textelArea = 4.0f * M_PI / 6.0f / (envMapSize * envMapSize);

  vec3 color = vec3(0.0f);
  float totalWeight = 0.0f;
  for(int i = 0; i < params.samplesPerTexel; i++)
  {
    float seed = random(texCoord);
    vec2 samplerValue = hammersley2d(i,
                                     params.samplesPerTexel,
                                     uint(seed * 10 * params.samplesPerTexel));
    vec3 microNormal = sampleGGX(samplerValue, roughness_4);
    vec3 sampleDirection = reflect(vec3(0, 0, -1), microNormal);
    
    //  sampleDirection - это направление, с которого мы принимаем свет
    //  Здесь это направление в tbn базисе
    float normDotLight = sampleDirection.z;
    if(normDotLight > 0.0f)
    {
      //  Микронормаль - это и есть средний вектор между view и light
      float normDotHalf = microNormal.z;

      //  Вычисляем лод, из которого будем считывать яркость
      float pdf = ggxNormalDistribution(normDotHalf, roughness_2) / 4.0f;
      //  телесный угол, который должна охватывать наша выборка
      float sampleArea = 1.0 /
                              (float(params.samplesPerTexel) * (pdf + 0.0001f));
      float mipLevel = roughness == 0 ?
                        0 :
                        max(0.5f * log2(sampleArea / textelArea) + 1.0f,
                            0.0f);

      //  Переводим направление сэмплирования из tbn базиса в мировые координаты
      sampleDirection = tbn * sampleDirection;
      sampleDirection.z = -sampleDirection.z;

      vec3 ambientValue = textureLod(samplerCube(environmentMap, colorSampler),
                          sampleDirection,
                          mipLevel).rgb;

      color += ambientValue * normDotLight;
      totalWeight += normDotLight;
    }
  }

  outColor = vec4(color / totalWeight, 1.0);
}