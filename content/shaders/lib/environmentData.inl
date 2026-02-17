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
};

#endif