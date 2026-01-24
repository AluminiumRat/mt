#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/globalLightData.inl>

layout (set = COMMON, binding = 0) uniform CameraDataBlock
{
  CameraData value;
} cameraData;

layout (set = COMMON, binding = 1) uniform GlobalLightDataBlock
{
  GlobaLightData value;
} globalLightData;

#endif