#version 450

#include "textureViewer/viewer.inl"

vec4 positions[36] = vec4[](vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4(-1.0f,  1.0f, -1.0f, 1.0f),
                            vec4( 1.0f,  1.0f, -1.0f, 1.0f),
                            vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f,  1.0f, -1.0f, 1.0f),
                            vec4( 1.0f, -1.0f, -1.0f, 1.0f),

                            vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                            vec4( 1.0f, -1.0f, 1.0f, 1.0f),
                            vec4( 1.0f,  1.0f, 1.0f, 1.0f),
                            vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                            vec4( 1.0f,  1.0f, 1.0f, 1.0f),
                            vec4(-1.0f,  1.0f, 1.0f, 1.0f),

                            vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4(-1.0f, -1.0f,  1.0f, 1.0f),
                            vec4(-1.0f,  1.0f,  1.0f, 1.0f),
                            vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4(-1.0f,  1.0f,  1.0f, 1.0f),
                            vec4(-1.0f,  1.0f, -1.0f, 1.0f),

                            vec4( 1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f,  1.0f, -1.0f, 1.0f),
                            vec4( 1.0f,  1.0f,  1.0f, 1.0f),
                            vec4( 1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f,  1.0f,  1.0f, 1.0f),
                            vec4( 1.0f, -1.0f,  1.0f, 1.0f),

                            vec4(-1.0f, 1.0f, -1.0f, 1.0f),
                            vec4(-1.0f, 1.0f,  1.0f, 1.0f),
                            vec4( 1.0f, 1.0f,  1.0f, 1.0f),
                            vec4(-1.0f, 1.0f, -1.0f, 1.0f),
                            vec4( 1.0f, 1.0f,  1.0f, 1.0f),
                            vec4( 1.0f, 1.0f, -1.0f, 1.0f),

                            vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f, -1.0f,  1.0f, 1.0f),
                            vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                            vec4( 1.0f, -1.0f,  1.0f, 1.0f),
                            vec4(-1.0f, -1.0f,  1.0f, 1.0f));

layout(location = 0) out vec3 cubemapDirection;

void main()
{
  gl_Position = renderParams.viewProjMatrix * positions[gl_VertexIndex];
  cubemapDirection = positions[gl_VertexIndex].xyz;
}