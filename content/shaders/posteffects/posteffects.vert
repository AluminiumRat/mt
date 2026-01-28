#version 450

vec4 positions[4] = vec4[]( vec4(-1.0f, -1.0f, 0.0f, 1.0f),
                            vec4(-1.0f,  1.0f, 0.0f, 1.0f),
                            vec4( 1.0f, -1.0f, 0.0f, 1.0f),
                            vec4( 1.0f,  1.0f, 0.0f, 1.0f));

layout(location = 0) out vec2 outTexCoord;

void main()
{
  gl_Position = positions[gl_VertexIndex];
  outTexCoord = positions[gl_VertexIndex].xy * 0.5f + 0.5f;
}