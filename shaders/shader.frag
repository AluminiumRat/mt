#version 450

layout (set = 1,
        binding = 2) uniform ColorData
{
    vec4 color;
} colorData;

layout (set = 1,
        binding = 3) uniform texture2D colorTexture;
layout (set = 1,
        binding = 4) uniform sampler samplerState;

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture( sampler2D(colorTexture, samplerState),
                        texCoord);
    //outColor = colorData.color * vec4(vertexColor, 1.0);
}