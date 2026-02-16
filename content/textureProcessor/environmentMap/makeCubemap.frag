#version 450
//  Шейдер переводит карту окружения из равнопромежуточной проекции(spheremap)
//  в cubemap-у

#include "lib/cubemap.inl"

layout (set = STATIC) uniform IntrinsicData
{
  uint arrayIndex;
} intrinsic;

layout (set = STATIC) uniform texture2D colorTexture;
layout (set = STATIC) uniform sampler colorSampler;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
  //  Получаем направление сэмплирования в мировых координатах
  vec3 sampleDirection = cubemapDirection(texCoord, intrinsic.arrayIndex);

  //  Переводим направление сэмплирования в широту/долготу, а из них
  //  в текстурные координаты сферамапы
  float lon = 0;
  if(sampleDirection.x != 0.0f || sampleDirection.y != 0.0f)
  {
    float lonCos = sampleDirection.x / length(sampleDirection.xy);
    lon = acos(lonCos);
    if(sampleDirection.y < 0) lon = 2.0f * M_PI - lon;
  }
  float uCoord = 1.0f - lon / (2.0f * M_PI);

  float latSin = sampleDirection.z;
  float lat = asin(latSin);
  float vCoord = 0.5f - lat / M_PI;

  outColor = texture( sampler2D(colorTexture, colorSampler),
                      vec2(uCoord, vCoord));
}