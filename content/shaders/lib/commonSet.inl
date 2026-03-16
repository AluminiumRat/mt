#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/environmentData.inl>

layout (set = COMMON, binding = 0) uniform CommonData
{
  CameraData cameraData;
  EnvironmentData environment;
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

//  Скринспейсовый буфер с тенями
layout (set = COMMON,
        binding = 5) uniform texture2D shadowBuffer;

//  Дефолтный сэмплер общего назначения.
//  Линейная фильтрация, VK_SAMPLER_ADDRESS_MODE_REPEAT, анизотропия 4
layout (set = COMMON,
        binding = 6) uniform sampler commonLinearSampler;

#endif