//  Набор процедур для работы с цветом
#ifndef COLOR_INL
#define COLOR_INL

//  Преобразование из RGB спекта в яркость
//  Просто взвешивание компонентов по восприятию глазом
float colorToLuminance(vec3 color)
{
  return dot(color, vec3(0.3f, 0.6f, 0.1f));
}

//  Преобразование Реинхарда (Extended Reinhard tone-mapping operator)
vec3 reinhard(vec3 x, float maxWhiteSq)
{
  return x * (1.0f + x / maxWhiteSq) / (1.0f + x);
}

#endif