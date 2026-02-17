#ifndef ENVIRONMENT_DATA_INL
#define ENVIRONMENT_DATA_INL

//  Юниформ данные, которые поставляются классом EnvironmentScene
struct EnvironmentData
{
  vec3 fromSunDirection;
  vec3 toSunDirection;
  //  Освещенность от солнца.
  //  Измеряется на поверхности, перпендикулярной направлению света
  vec3 directLightIrradiance;
  //  Коэффициент для перевода рафнеса материала в номер лода спекулар мапы
  //  (для IBL освещения)
  float roughnessToLod;
};

#endif