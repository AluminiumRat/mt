#version 450
//  Расчет LUT текстуры для image based lighting
//  Основано на https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//  Основное отличие - добавлен третий канал, в котором накапливается нормировочный коэффициент для
//  ламбертового отражения.

#include "lib/brdf.inl"
#include "lib/random.inl"

layout (set = STATIC, binding = 1) uniform Params
{
  uint samplesCount;
} params;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  float normDotView = texCoord.x;

  float roughness = texCoord.y;
  float roughness_2 = roughness * roughness;
  float roughness_4 = roughness_2 * roughness_2;

  vec3 viewDir = vec3(1.0f - normDotView * normDotView,
                      0.0f,
                      normDotView);

  //  A и B - взяты из оригенальной работы. Здесь накапливаются коэффициенты
  //  для вычисления спекуляр отражения
  float A = 0;
  float B = 0;
  //  В C накапливается нормировочный коэффициент для Ламбертового отражения
  //  В gltf модели освещения Ламберт и спекуляр смешиваются по Френелю, поэтому
  //    в C складывается среднее значение френеля (вернее 1 - fresnel)
  float C = 0;
  for(uint i = 0; i < params.samplesCount; i++)
  {
    vec2 samplerValue = hammersley2d(i, params.samplesCount,0);
    vec3 microNormal = sampleGGX(samplerValue, roughness_4);
    vec3 lightDir = reflect(-viewDir, microNormal);

    //  Получаем все нужные дот продукты, учитывая, что макронормаль у нас
    //  (0,0,1), а halfVector - это микронормаль
    float normDotLight = lightDir.z;
    float normDotHalf = microNormal.z;
    float viewDotHalf = max(dot(viewDir, microNormal), 0.0f);
    if(normDotLight > 0.0f)
    {
      float G = geometrySchlickSmith(normDotLight, normDotView, roughness_2);
      float G_Vis = (G * viewDotHalf) / (normDotHalf * normDotView);
      float Fc = fresnelSlickCompliment(viewDotHalf);
      A += (1.0f - Fc) * G_Vis;
      B += Fc * G_Vis;
      C += (1.0f - dielectricFresnel(Fc));
    }
  }

  outColor = vec4(A / float(params.samplesCount),
                  B / float(params.samplesCount),
                  C / float(params.samplesCount),
                  1);
}