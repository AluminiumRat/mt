layout (set = VOLATILE,
        binding = 0) uniform RenderParams
{
  mat4 viewProjMatrix;
  mat4 modelMatrix;
} renderParams;

layout (set = STATIC,
        binding = 0) uniform texture2DArray colorTexture;
layout (set = STATIC,
        binding = 1) uniform sampler textureSampler;