#include <lib/cameraData.inl>

layout (set = COMMON,
        binding = 0) uniform CameraDataBlock
{
  CameraData value;
} cameraData;

layout (set = VOLATILE,
        binding = 0) uniform TransformData
{
  mat4 positionMatrix[32];
  //mat3 bivecMatrix[32];
  //mat4 prevPositionMatrix[32];
} transformData;

struct Vertex
{
  vec3 position;
};
layout (set = STATIC,
        binding = 0) readonly buffer VertexBuffer
{
  Vertex data[];
} vertices;

layout (set = STATIC,
        binding = 1) uniform texture2D colorTexture;
layout (set = STATIC,
        binding = 2) uniform sampler samplerState;