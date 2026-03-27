#ifndef RAY_QUERY_SHADOWS_INL
#define RAY_QUERY_SHADOWS_INL

#include "lib/commonSet.inl"

//  Нулевой биндинг зарезервирован за TLAS

layout (set = STATIC, binding = 1) uniform Params
{
  //  Смещения начала луча при рэй трэйсинге
  float rayForwardShift;
  float rayNormalShift;
} params;

//  Белый шум 32х32 пикселя
layout (set = STATIC, binding = 2) uniform texture2D noiseTexture;
//  Сэмплы для равномерной выборки по диску
layout (set = STATIC, binding = 3) uniform texture1D samplerTexture;

//  Буфер, куда кладется история результатов трассировки теней.
//  r канал - текущая трассировка, g - предыдущая и т.д. Всего 4 кадра
layout(rgba8, set = VOLATILE, binding = 0) uniform image2D traceResults;
//  История трассировки с предыдущего кадра.
//  r канал - предыдущая трассировка, g - 2 кадра назад и т.д. Всего 4 кадра
layout(set = VOLATILE, binding = 1) uniform texture2D traceResultsPrev;

//  Сюда склыдываем окончательную отфильтрованную маску теней
layout(r8, set = STATIC, binding = 4) uniform image2D finalShadowMask;

#endif