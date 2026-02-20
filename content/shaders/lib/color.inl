//  Набор процедур для работы с цветом
#ifndef COLOR_INL
#define COLOR_INL

//  Грубый перевод из sRGB в линейное пространство
vec3 srgbToLinearFast(vec3 srgbValue)
{
  return pow(srgbValue, vec3(2.2f));
}

//  Грубый перевод из линейного пространства в sRGB
vec3 linearToSrgbFast(vec3 linearValue)
{
  return pow(linearValue, vec3(1.0f / 2.2f));
}

//  Преобразование из RGB спекта в яркость
//  Просто взвешивание компонентов по восприятию глазом
float colorToLuminance(vec3 color)
{
  return dot(color, vec3(0.3f, 0.6f, 0.1f));
}

//  Делает цвет с тем же соотношением RGB компонент, что и
//  color, но с яркостью, равной luminanceOut
//  Нельзя передавать нулевой color
vec3 changeLuminance(vec3 color, float luminanceOut)
{
  float luminanceIn = colorToLuminance(color);
  return luminanceOut / luminanceIn * color;
}

//  Преобразование Реинхарда (Extended Reinhard tone-mapping operator)
vec3 reinhardEx(vec3 x, float maxWhiteSq)
{
  return x * (1.0f + x / maxWhiteSq) / (1.0f + x);
}
float reinhardEx(float x, float maxWhiteSq)
{
  return x * (1.0f + x / maxWhiteSq) / (1.0f + x);
}

#endif