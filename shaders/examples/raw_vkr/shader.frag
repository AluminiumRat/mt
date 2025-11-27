#version 450

layout (binding = 2) uniform ColorData
{
    vec4 color;
} colorData;

layout(binding = 3) uniform texture2D colorTexture1;
layout(binding = 4) uniform sampler samplerState;

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture( sampler2D(colorTexture1, samplerState),
                        texCoord);
    outColor *= colorData.color * vec4(vertexColor, 1.0);
}