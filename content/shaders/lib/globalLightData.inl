#ifndef GLOBAL_LIGHT_DATA_INL
#define GLOBAL_LIGHT_DATA_INL

struct GlobaLightData
{
  vec3 fromSunDirection;
  vec3 toSunDirection;
  //  Освещенность поверхности, перпендикулярной направлению света
  vec3 directLightIrradiance;
};

#endif