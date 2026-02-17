#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/environmentData.inl>

layout (set = COMMON, binding = 0) uniform CommonData
{
  CameraData cameraData;
  EnvironmentData environmentData;
} commonData;

//  LUT текстура для image based lighting и сэмплер для её чтения
layout (set = COMMON,
        binding = 1) uniform texture2DArray iblLut;
layout (set = COMMON,
        binding = 2) uniform sampler iblLutSampler;

#endif