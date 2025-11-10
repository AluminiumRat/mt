#version 450

layout (set = 1,
        binding = 2) uniform ColorData
{
    vec4 color;
} colorData;


layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    //outColor = vec4(fragColor, 1.0);
    outColor = colorData.color;
}