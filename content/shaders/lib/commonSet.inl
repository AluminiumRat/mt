#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/environmentData.inl>

layout (set = COMMON, binding = 0) uniform CommonData
{
  CameraData cameraData;
  EnvironmentData environmentData;
} commonData;

#endif