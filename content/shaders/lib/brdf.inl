#ifndef BRDF_INL
#define BRDF_INL

//  Индекс преломления для диэлектриков
#define DIELECTRIC_IOR 1.5f
//  Коэффициент отражения Френеля под углом 90 градусов для диэлектриков
#define DIELECTRIC_F0 0.04f

//  GGX функция распределения нормалей. D - компонент в модели Кука-Торренса
//  normDotHalf - косинус угла между нормалью и средним между вью и вектором
//    света
//  roughness2 - квадрат roughness, или alpha в классическом обозначении
float ggxNormalDistribution(float normDotHalf, float roughness2)
{
  float roughness4 = roughness2 * roughness2;
  float factor = roughness4 / M_PI;
  float denom = normDotHalf * normDotHalf * (roughness4 - 1.f) + 1.f;
  denom = denom * denom;
  factor /= denom;
  return factor;
}

//  Функция самозатенения Шлика-Смитта(GGX). G - компоненты в модели
//    Кука-Торренса
//  roughness2 - квадрат roughness
float geometrySchlickSmith( float normDotLight,
                            float normDotView,
                            float roughness2)
{
  float k = roughness2 / 2.0f;
  float GL = normDotLight / (normDotLight * (1.0f - k) + k);
  float GV = normDotView / (normDotView * (1.0f - k) + k);
  return GL * GV;
}

//  Функция видимости. Часть модели Кука-Торренса
//  Heitz, E. (2014): Understanding the Masking-Shadowing Function in
//    Microfacet-Based BRDFs
float visibilityFunction(float normDotLight, float normDotView)
{
  return 1.f / (4.0f * normDotLight * normDotView);
}

//  Объединение самозатенения Шлика-Смитта и функции видимости для
//  уменьшения вычислений
float schlickSmithGeometryVisibility( float normDotLight,
                                      float normDotView,
                                      float roughness2)
{
  float k = roughness2 / 2.0f;
  float GL = normDotLight * (1.0f - k) + k;
  float GV = normDotView * (1.0f - k) + k;
  return 1.0f / 4.0f / (GL * GV);
}                        

//  BRDF по Куку-Торренсу, но без отражения по Френелю
float specularBRDF( float normDotLight,
                    float normDotView,
                    float normDotHalf,
                    float roughness2)
{
  //  Закоменчен более понятный, но немного более медленный вариант "как по
  //  букварю". Оставлен для контроля и понимания, откуда что пришло.
  /*return ggxNormalDistribution(normDotHalf, roughness2) *
          geometrySchlickSmith(normDotLight, normDotView, roughness2) *
            visibilityFunction(normDotLight, normDotView);*/
  return ggxNormalDistribution(normDotHalf, roughness2) *
          schlickSmithGeometryVisibility(normDotLight, normDotView, roughness2);
}

//  Диффузное отражение. BRDF по Ламберту. Равномерное отражение во все стороны
float lambertBRDF()
{
  return 1.0f / M_PI;
}

//  Вспомогательная величина для вычисления отражений по Френелю(аппроксимация Шлика)
//  (1 - viewDotHalf)^5 для viewDotHalf > 0
float fresnelSlickCompliment(float viewDotHalf)
{
  float compliment = min(1.0f - viewDotHalf, 1.0f);
  float compliment2 = compliment * compliment;
  return compliment2 * compliment2 * compliment;
}

//  Коэффициент отражения для диэлектриков. Френель, аппроксимация Шлика
//  compliment можно получить с помощью fresnelSlickCompliment
float dielectricFresnel(float compliment)
{
  return DIELECTRIC_F0 + (1.0f - DIELECTRIC_F0) * compliment;
}

//  Аппроксимация Шлика для коррекции отражения металлов в зависимости от
//  направления наблюдателя
//  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#fresnel
//  compliment можно получить с помощью fresnelSlickCompliment
vec3 metallicReflectionFactor(vec3 baseColor, float compliment)
{
  return baseColor + (vec3(1.0f) - baseColor) * compliment;
}

//  BRDF для диэлектриков по GLTF2 модели
//  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf
//  fresnelCompliment можно получить с помощью fresnelSlickCompliment
//  specularBRDFValue можно получить с помощью specularBRDF
vec3 dielectricBRDF(vec3 baseColor,
                    float specularBRDFValue,
                    float fresnelCompliment)
{
  return mix( lambertBRDF() * baseColor,
              vec3(specularBRDFValue),
              dielectricFresnel(fresnelCompliment));
}

//  BRDF для металлов по GLTF2 модели
//  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf
//  fresnelCompliment можно получить с помощью fresnelSlickCompliment
//  specularBRDFValue можно получить с помощью specularBRDF
vec3 metallicBRDF(vec3 baseColor,
                  float specularBRDFValue,
                  float fresnelCompliment)
{
  return specularBRDFValue * metallicReflectionFactor(baseColor, fresnelCompliment);
}

//  BRDF для GLTF2 материалов.
//  Медленный вариант. На боевую не используется. Оставлен в качестве эталона и для
//    понимания, как работает ускоренный вариант
//  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf
vec3 gltf2BRDF( vec3 baseColor,
                float roughness,
                float metallic,
                float normDotLight,
                float normDotView,
                float normDotHalf,
                float viewDotHalf)
{
  float roughness2 = roughness * roughness;
  float specularBRDFValue = specularBRDF( normDotLight,
                                          normDotView,
                                          normDotHalf,
                                          roughness2);
  float fresnelCompliment = fresnelSlickCompliment(viewDotHalf);
  vec3 dielectricBRDFValue = dielectricBRDF(baseColor,
                                            specularBRDFValue,
                                            fresnelCompliment);
  vec3 metallicBRDFValue = metallicBRDF(baseColor,
                                        specularBRDFValue,
                                        fresnelCompliment);
  return mix( dielectricBRDFValue,
              metallicBRDFValue,
              metallic);
}

//  BRDF для GLTF2 материалов. Ускоренный вариант. Ускорение за счет объединения
//  общих рассчетов.
//  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf
vec3 glt2BRDFFast(vec3 baseColor,
                  float roughness,
                  float metallic,
                  float normDotLight,
                  float normDotView,
                  float normDotHalf,
                  float viewDotHalf)
{
  //  Ламбертово отражение работает только на диэлектриках
  vec3 lambert = lambertBRDF() * baseColor * (1.0f - metallic);

  //  GGX БРДФ одинаковая и для металла и для диэлектрика, отличия в спектре и
  //  работе Френеля
  vec3 specular = vec3(specularBRDF(normDotLight,
                                    normDotView,
                                    normDotHalf,
                                    roughness * roughness));

  //  За счет того, что и металл и диэлектрик одинаково переходят в белое отражение на
  //  тупых углах, мы можем протащить весь расчет френеля через metallicReflectionFactor
  vec3 f0Reflection = mix(vec3(DIELECTRIC_F0), baseColor, metallic);
  float fresnelCompliment = fresnelSlickCompliment(viewDotHalf);
  vec3 fresnel = metallicReflectionFactor(f0Reflection, fresnelCompliment);

  return mix(lambert, specular, fresnel);
}

#endif