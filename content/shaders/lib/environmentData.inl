#ifndef ENVIRONMENT_DATA_INL
#define ENVIRONMENT_DATA_INL

//  Юниформ данные, которые поставляются классом EnvironmentScene
struct EnvironmentData
{
  vec3 fromSunDirection;
  vec3 toSunDirection;

  //  Вектор, перпендикулярный toSunDirection и линии горимзонта
  vec3 sunLongitudeDirection;
  //  Вектор, перпендикулярный toSunDirection и параллельный горизонту
  vec3 sunLatitudeDirection;

  //  Угловой размер(диаметр) солнца в радианах
  float sunAngleSize;

  //  Освещенность от солнца.
  //  Измеряется на поверхности, перпендикулярной направлению света
  vec3 directLightIrradiance;
  //  Коэффициент для перевода рафнеса материала в номер лода спекулар мапы
  //  (для IBL освещения)
  float roughnessToLod;
};

#endif