#include <lib/commonSet.inl>

layout (set = VOLATILE,
        binding = 0) uniform TransformData
{
  mat4 positionMatrix[32];
  mat3 bivecMatrix[32];
  mat4 prevPositionMatrix[32];
} transformData;

#if INDICES_ENABLED == 1
  // Индексный буфер. По 1 индексу на вершину. Без разрывов
  layout (set = STATIC, binding = 0) readonly buffer IndexBuffer
  {
    int data[];
  } INDICES;
#endif

// Буфер с координатами вершин
// Координаты укладываются без разрывов по 3 значения для каждой вершины
layout (set = STATIC, binding = 1) readonly buffer PositionBuffer
{
  float data[];
} POSITION;

// Нормали
// Координаты нормалей укладываются без разрывов по 3 значения для каждой
// вершины
layout (set = STATIC, binding = 2) readonly buffer NormalBuffer
{
  float data[];
} NORMAL;

#if TEXCOORD_COUNT > 0
  // Текстурные координаты. По 2 float-а на вершину. Без разрывов
  layout (set = STATIC, binding = 3) readonly buffer Texcoord0Buffer
  {
    float data[];
  } TEXCOORD_0;
#endif

layout (set = STATIC, binding = 4) uniform sampler linearSampler;