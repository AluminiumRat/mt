#ifndef LIT_SURFACE_INL
#define LIT_SURFACE_INL

//  Коэффициент отражения Френеля под углом 90 градусов для диэлектриков
#define DIELECTRIC_F0 0.04f

//  Данные о поверхности, для которой рассчитывается освещение
//  Инициализируются через makeLitSurface
//  Мелкая рутина перед просчетом освещения
struct LitSurface
{
  vec3 baseColor;
  float roughness;
  float metallic;
  //  Дифузная составляющая цвета, темнее baseColor из-за отсутствия у металлов
  //  диффузной составляющей отражения
  vec3 lambertColor;
  //  Коэффициент зеркального отражения под углом 90
  vec3 f0Reflection;

  vec3 normal;
  vec3 toViewer;
  vec3 toLight;
  vec3 halfVector;
  float normDotLight;
  float normDotView;
  float normDotHalf;
  float viewDotHalf;
};

//  Инициализаия структуры LitSurface
LitSurface makeLitSurface(vec3 baseColor,
                          float roughness,
                          float metallic,
                          vec3 normal,
                          vec3 toViewer,
                          vec3 toLight)
{
  LitSurface surface;
  surface.baseColor = baseColor;
  surface.roughness = roughness;
  surface.metallic = metallic;
  surface.lambertColor = baseColor * (1.0f - metallic);
  surface.f0Reflection = mix( vec3(DIELECTRIC_F0),
                              baseColor.rgb,
                              metallic);

  surface.normal = normal;
  surface.toViewer = toViewer;
  surface.toLight = toLight;
  surface.halfVector = toViewer + toLight;
  surface.halfVector = normalize(surface.halfVector);
  surface.normDotLight = dot(toLight, normal);
  surface.normDotLight = max(surface.normDotLight, 0.001f);
  surface.normDotView = dot(toViewer, normal);
  surface.normDotView = max(surface.normDotView, 0.001f);
  surface.normDotHalf = dot(surface.halfVector, normal);
  surface.viewDotHalf = dot(toViewer, surface.halfVector);
  return surface;
}

#endif