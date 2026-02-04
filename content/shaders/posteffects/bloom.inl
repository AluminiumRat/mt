layout (set = VOLATILE,
        binding = 0) uniform texture2D sourceImage;

layout (set = STATIC,
        binding = 0) uniform sampler linearSampler;

layout( r11f_g11f_b10f,
        set = STATIC,
        binding = 1) uniform image2D targetImage;

layout (set = STATIC,
        binding = 3) readonly buffer AvgLuminance
{
  float value;
} avgLuminance;

layout (set = STATIC,
        binding = 4) uniform Params
{
  vec2 invSourceSize;
  ivec2 targetSize;
  float threshold;
  float intensity;
} params;

//  Сколько шагов смешивания будет производиться
#define BLOOR_STEPS 5

//  Вес центральной точки при размытии
#define CENTER_WEIGHT 0.5f
//  Вес каждого из соседей при размытии
#define NEIGHBOR_WEIGHT 0.25f