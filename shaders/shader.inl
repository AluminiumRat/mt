struct Vertex
{
    vec3 position;
    vec4 color;
};
layout (set = VOLATILE,
        binding = 1) readonly buffer VertexBuffer
{
    Vertex data[];
} vertices;

layout (set = VOLATILE,
        binding = 2) uniform ShiftData
{
    float shift;
} shiftData;

layout (set = VOLATILE,
        binding = 3) uniform ColorData
{
    float someData;
    vec4 color;
} colorData;

layout (set = VOLATILE,
        binding = 4) uniform texture2D colorTexture1;
layout (set = STATIC,
        binding = 5) uniform texture2D colorTexture2;
layout (set = STATIC,
        binding = 6) uniform sampler samplerState;