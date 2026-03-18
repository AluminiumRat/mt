#version 450

#include "lib/random.inl"

layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(random(gl_FragCoord.xy));
}