#version 450

#include "shader.inl"

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out vec3 outVertexColor;
//layout(location = 1) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outVertexColor = colors[gl_VertexIndex];

    //gl_Position = vec4(vertices.data[gl_VertexIndex].position, 1.0);
    //outVertexColor = vertices.data[gl_VertexIndex].color.rgb;
    //outTexCoord = vertices.data[gl_VertexIndex].position.xy;
}