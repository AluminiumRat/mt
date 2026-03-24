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

//  Здесь хранятся неотфильтрованные результаты трассировки лучей
layout(r8, set = VOLATILE, binding = 0) uniform image2D rawShadowMask;
//  Результаты трассировки лучей с предыдущего кадра
layout(set = VOLATILE, binding = 1) uniform texture2D prevShadowMask;

//  Сюда склыдываем окончательную отфильтрованную маску теней
layout(r8, set = STATIC, binding = 4) uniform image2D finalShadowMask;

#endif