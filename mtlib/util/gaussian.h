#pragma once

#include <cmath>

#include <util/Assert.h>
#include <util/pi.h>

namespace mt
{
  //  Несмещенная функция Гаусса
  inline float gaussian(float x, float sigma) noexcept
  {
    constexpr float gaussianFactor = 0.39894228040143267793994605993438f;
    float a = gaussianFactor / sigma;
    float exponent = -(x * x) / (2.0f * sigma * sigma);
    return a * exp(exponent);
  }

  //  Интеграл функции Гаусса методом трапеций
  inline float gaussianIntegral(float start,
                                float finish,
                                unsigned int stepsCount,
                                float sigma)
  {
    MT_ASSERT(stepsCount != 0);

    float step = (finish - start) / stepsCount;
    float currentX = start;
    float currentValue = gaussian(currentX, sigma);
    float result = 0.0f;
    for(unsigned int i = 0; i < stepsCount; i++)
    {
      float nextX = currentX + step;
      float nextValue = gaussian(nextX, sigma);
      result += (currentValue + nextValue) / 2.0f * step;
      currentX = nextX;
      currentValue = nextValue;
    }
    return result;
  }
}