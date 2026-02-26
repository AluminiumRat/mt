layout (set = STATIC,
        binding = 0) uniform texture2D sourceImage;

layout (set = STATIC,
        binding = 1) uniform sampler linearSampler;

layout( r32f,
        set = STATIC,
        binding = 2) uniform image2D intermediateImage;

layout( set = STATIC,
        binding = 3) buffer AvgLumBuffer
{
  float value;
} avgLumBuffer;

layout (set = VOLATILE,
        binding = 4) uniform Params
{
  //  1.0f / размер исходного изображения
  vec2 invSourceSize;
  //  Размер сетки, по которой работают компьют шейдеры
  ivec2 areaSize;
  //  Сколько пикселей по горизонтали и вертикали приходится на 1 вызов
  //  горизонтального шейдера
  vec2 pixelRate;
  //  В какой пропорции надо смешивать яркость, полученную на текущем
  //  кадре, со значениями предыдущих кадров
  float mixFactor;
} params;
