#version 450

#include "gltf/gltfFragCommon.inl"
#include "lib/octahedronEncoding.inl"

layout(location = 0) out float outLinearDepth;
layout(location = 1) out vec2 outNormal;
layout(location = 2) out float outRoughness;

void main()
{
  outLinearDepth = inWorldPosition.w;
  outNormal = octahedronEncode(getNormal());
  outRoughness = getRoughnessMetallic().r;
}