#version 450

#include "gltf/gltfFragCommon.inl"
#include "lib/octahedronEncoding.inl"

layout(location = 0) out vec2 outNormal;

void main()
{
  outNormal = octahedronEncode(getNormal());
}