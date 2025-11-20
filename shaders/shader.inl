struct Vertex
{
    vec3 position;
    vec4 color;
};
layout (set = STATIC,
        binding = 1) readonly buffer VertexBuffer
{
    Vertex data[];
} vertices;

layout (set = VOLATILE,
        binding = 2) uniform ColorData
{
    vec4 color;
} colorData;

layout (set = STATIC,
        binding = 3) uniform texture2D colorTexture1;
layout (set = STATIC,
        binding = 4) uniform texture2D colorTexture2;
layout (set = STATIC,
        binding = 5) uniform sampler samplerState;