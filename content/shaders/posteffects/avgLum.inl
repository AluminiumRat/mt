layout (set = VOLATILE,
        binding = 0) uniform texture2D sourceImage;

layout (set = STATIC,
        binding = 0) uniform sampler linearSampler;

layout( r32f,
        set = STATIC,
        binding = 1) uniform image2D intermediateImage;

layout( set = STATIC,
        binding = 2) writeonly buffer AvgLumBuffer
{
  float value;
} avgLumBuffer;

layout (set = STATIC,
        binding = 3) uniform Params
{
  vec2 invSourceSize;
  ivec2 areaSize;
} params;
