#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/environmentData.inl>

layout (set = COMMON, binding = 0) uniform CommonData
{
  CameraData cameraData;
  EnvironmentData environment;
  //  Размер области, в которую отрисовывается конечная картинка
  //  zw = 1 / xy
  vec4 frameBufferExtent;
  //  Размер половинных буферов
  //  zw = 1 / xy
  vec4 halfFrameExtent;
} commonData;

//  LUT текстура для image based lighting и сэмплер для её чтения
layout (set = COMMON,
        binding = 1) uniform texture2D iblLut;
layout (set = COMMON,
        binding = 2) uniform sampler iblLutSampler;

//  Префильтрованная Irradiance текстура для IBL освещения
layout (set = COMMON,
        binding = 3) uniform textureCube iblIrradianceMap;

//  Префильтрованная specular текстура для IBL освещения
layout (set = COMMON,
        binding = 4) uniform textureCube iblSpecularMap;

//  Линейный буфер глубины в половинном разрешении
layout (set = COMMON,
        binding = 5) uniform texture2D linearDepthHalfBuffer;

//  Буфер нормалей в половинном разрешении
layout (set = COMMON,
        binding = 6) uniform texture2D normalHalfBuffer;

//  Скринспейсовый буфер с тенями
layout (set = COMMON,
        binding = 7) uniform texture2D shadowBuffer;

//  Дефолтный сэмплер общего назначения.
//  Линейная фильтрация, VK_SAMPLER_ADDRESS_MODE_REPEAT, анизотропия 4
layout (set = COMMON,
        binding = 8) uniform sampler commonLinearSampler;

//  Получить размер пикселя для половинных буферов на определенной дистанции
//  от камеры
float getHalfBufferPixelSize(float linearDepth)
{
  return commonData.cameraData.fovY * linearDepth *
                                                  commonData.halfFrameExtent.w;
}

#endif