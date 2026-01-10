//  Библиотечный шейдер
//  Здесь находятся общие настройки ресурсов, которые используются
//    в shader.frag и shader.vert

//  Вершинны пробрасываются через readonly буфер, а не через
//  input assembler
struct Vertex
{
    vec3 position;
    vec4 color;
};
layout (set = STATIC,
        binding = 0) readonly buffer VertexBuffer
{
    Vertex data[];
} vertices;

layout (set = VOLATILE,
        binding = 0) uniform ColorData
{
    vec4 color;
} colorData;

layout (set = STATIC,
        binding = 1) uniform texture2D colorTexture;
layout (set = STATIC,
        binding = 2) uniform sampler samplerState;