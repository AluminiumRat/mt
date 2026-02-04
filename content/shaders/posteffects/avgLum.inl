layout (set = STATIC,
        binding = 0) uniform texture2D sourceImage;

layout (set = STATIC,
        binding = 1) uniform sampler linearSampler;

layout( r32f,
        set = STATIC,
        binding = 2) uniform image2D intermediateImage;

layout( set = STATIC,
        binding = 3) writeonly buffer AvgLumBuffer
{
  float value;
} avgLumBuffer;

layout (set = STATIC,
        binding = 4) uniform Params
{
  vec2 invSourceSize;   // 1.0f / размер исходного изображения
  ivec2 areaSize;       // Размер сетки, по которой работают компьют шейдеры
  vec2 pixelRate;       // Сколько пикселей по горизонтали и вертикали
                        // приходится на 1 вызов горизонтального шейдера
} params;
