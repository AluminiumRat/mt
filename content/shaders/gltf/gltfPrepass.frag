#version 450

#include "gltf/gltfFragCommon.inl"

layout(location = 0) out vec2 outNormal;

void main()
{
  vec3 normal = getNormal();
  outNormal = normal.xy;
}