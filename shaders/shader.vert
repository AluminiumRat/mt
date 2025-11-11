#version 450

struct Vertex
{
    vec3 position;
    vec4 color;
};
layout (set = 1,
        binding = 1) readonly buffer VertexBuffer
{
    Vertex data[];
} vertices;

layout(location = 0) out vec3 outVertexColor;

void main()
{
    gl_Position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
    outVertexColor = vertices.data[gl_VertexIndex].color.rgb;
}