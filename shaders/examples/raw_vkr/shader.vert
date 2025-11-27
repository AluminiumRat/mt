#version 450

struct Vertex
{
    vec3 position;
    vec4 color;
};
layout(binding = 1) readonly buffer VertexBuffer
{
    Vertex data[];
} vertices;

layout(location = 0) out vec3 outVertexColor;
layout(location = 1) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
    outVertexColor = vertices.data[gl_VertexIndex].color.rgb;
    outTexCoord = vertices.data[gl_VertexIndex].position.xy;
}