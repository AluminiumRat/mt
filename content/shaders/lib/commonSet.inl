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

//  Дефолтный сэмплер общего назначения.
//  Линейная фильтрация, кламп к границам, анизотропия 4
layout (set = COMMON,
        binding = 5) uniform sampler commonLinearSampler;

#endif