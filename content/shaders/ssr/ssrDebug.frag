#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

//  Альфа блендинг в outColor
void addColor(vec4 color)
{
  outColor.rgb = mix(outColor.rgb, color.rgb, color.a);
  outColor.a += (1.0f - outColor.a) * color.a;
}

void main()
{
  outColor = vec4(0.0f, 0.0f, 0.0f, 0.1f);
  if(length(texCoord - vec2(0.5f)) < 0.002f) addColor(vec4(1.0f));
}