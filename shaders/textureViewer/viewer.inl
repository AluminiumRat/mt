layout (set = VOLATILE,
        binding = 0) uniform RenderParams
{
  mat4 viewProjMatrix;
  mat4 modelMatrix;
  float brightness;
  float mipIndex;
  float layer;
} renderParams;

layout (set = STATIC,
        binding = 0) uniform texture2DArray colorTexture;

layout (set = STATIC,
        binding = 1) uniform sampler nearestSampler;

layout (set = STATIC,
        binding = 2) uniform sampler linearSampler;