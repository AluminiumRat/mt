#ifndef LIT_SURFACE_INL
#define LIT_SURFACE_INL

//  Коэффициент отражения Френеля под углом 90 градусов для диэлектриков
#define DIELECTRIC_F0 0.04f

//  Данные об поверхности на которую мы смотрим
//  Инициализируются через makeObservedSurface
//  Просто группировка данных и мелкие рутины перед просчетом освещения
struct ObservedSurface
{
  vec3 baseColor;
  float roughness;
  float metallic;
  float ambientOcclusion;
  float specularOcclusion;
  //  Дифузная составляющая цвета, темнее baseColor из-за отсутствия у металлов
  //  диффузной составляющей отражения
  vec3 lambertColor;
  //  Коэффициент зеркального отражения под углом 90
  vec3 f0Reflection;

  vec3 normal;
  vec3 toViewer;
  float normDotView;
};

//  Дополнение к ObservedSurface, которое включает в себя информацию об
//    источнике прямого освещения
//  Инициализируются через makeLitSurface
//  Просто группировка данных и мелкие рутины перед просчетом освещения
struct LitSurface
{
  //  направление на источник света
  vec3 toLight;
  //  средний вектор между ObservedSurface::toViewer и LitSurface::toLight
  vec3 halfVector;

  float normDotLight;
  float normDotHalf;
  float viewDotHalf;
};

//  Инициализаия структуры ObservedSurface
ObservedSurface makeObservedSurface(vec3 baseColor,
                                    float roughness,
                                    float metallic,
                                    float ambientOcclusion,
                                    vec3 normal,
                                    vec3 toViewer)
{
  ObservedSurface surface;
  surface.baseColor = baseColor;
  surface.roughness = roughness;
  surface.metallic = metallic;
  surface.ambientOcclusion = ambientOcclusion;
  surface.lambertColor = baseColor * (1.0f - metallic);
  surface.f0Reflection = mix( vec3(DIELECTRIC_F0),
                              baseColor.rgb,
                              metallic);

  surface.normal = normal;
  surface.toViewer = toViewer;
  surface.normDotView = dot(toViewer, normal);
  surface.normDotView = max(surface.normDotView, 0.001f);

  //  specular occlusion взят из "Moving Frostbite to Physically Based Rendering 3.0"
  surface.specularOcclusion = pow(surface.normDotView + ambientOcclusion,
                                  exp2(-16.0f * roughness * roughness - 1.0f));
  surface.specularOcclusion = surface.specularOcclusion - 1.0f +
                                                              ambientOcclusion;
  surface.specularOcclusion = clamp(surface.specularOcclusion, 0.0f, 1.0f);
  return surface;
}

//  Инициализаия структуры LitSurface
LitSurface makeLitSurface(ObservedSurface observedSurface, vec3 toLight)
{
  LitSurface surface;
  surface.toLight = toLight;
  surface.halfVector = observedSurface.toViewer + toLight;
  surface.halfVector = normalize(surface.halfVector);
  surface.normDotLight = dot(toLight, observedSurface.normal);
  surface.normDotLight = max(surface.normDotLight, 0.001f);
  surface.normDotHalf = dot(surface.halfVector, observedSurface.normal);
  surface.viewDotHalf = dot(observedSurface.toViewer, surface.halfVector);
  return surface;
}

#endif