#version 450

#include "examples/technique/shader.inl"

layout(location = 0) out vec3 outVertexColor;
layout(location = 1) out vec2 outTexCoord;

void main()
{
    vec3 position = vertices.data[gl_VertexIndex].position;
    float angle = rotation.value;
    position.xy = vec2( position.x * cos(angle) - position.y * sin(angle),
                        position.y * cos(angle) + position.x * sin(angle));
    gl_Position = vec4(position, 1.0);
    outVertexColor = vertices.data[gl_VertexIndex].color.rgb;
    outTexCoord = vertices.data[gl_VertexIndex].position.xy;
}