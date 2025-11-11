#version 450

layout (set = 1,
        binding = 2) uniform ColorData
{
    vec4 color;
} colorData;


layout(location = 0) in vec3 vertexColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = colorData.color * vec4(vertexColor, 1.0);
}