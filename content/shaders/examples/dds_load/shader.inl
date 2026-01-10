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