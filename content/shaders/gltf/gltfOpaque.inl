#include <lib/commonSet.inl>
#include <gltf/gltfMaterial.inl>

layout (set = VOLATILE, binding = 0) uniform TransformData
{
  mat4 positionMatrix[32];
  mat3 bivecMatrix[32];
  mat4 prevPositionMatrix[32];
} transformData;

layout (set = STATIC, binding = 0) readonly buffer MaterialBuffer
{
  GLTFMaterial material;
} materialBuffer;

#if INDICES_ENABLED == 1
  // Индексный буфер. По 1 индексу на вершину. Без разрывов
  layout (set = STATIC, binding = 1) readonly buffer IndexBuffer
  {
    int data[];
  } INDICES;
#endif

// Буфер с координатами вершин
// Координаты укладываются без разрывов по 3 значения для каждой вершины
layout (set = STATIC, binding = 2) readonly buffer PositionBuffer
{
  float data[];
} POSITION;

// Нормали
// Координаты нормалей укладываются без разрывов по 3 значения для каждой
// вершины
layout (set = STATIC, binding = 3) readonly buffer NormalBuffer
{
  float data[];
} NORMAL;

//  Тангенты
//  vec4 на каждую вершину. XYZ - нормализованный вектор тангента,
//    W - знак(+1 или -1), указывающий на "рукость" tbn базиса
//  Тангенты используются только для текстур нормалей
#ifdef NORMALTEXTURE_ENABLED
  layout (set = STATIC, binding = 4) readonly buffer TangentBuffer
  {
    vec4 data[];
  } TANGENT;
#endif

#if BASECOLORTEXTURE_ENABLED == 1
  layout (set = STATIC, binding = 5) uniform texture2D baseColorTexture;
#endif

#if METALLICROUGHNESSTEXTURE_ENABLED == 1
  layout (set = STATIC,
          binding = 6) uniform texture2D metallicRougghnessTexture;
#endif

#ifdef NORMALTEXTURE_ENABLED
  layout (set = STATIC, binding = 7) uniform texture2D normalTexture;
#endif

#ifdef OCCLUSIONTEXTURE_ENABLED
  layout (set = STATIC, binding = 8) uniform texture2D occlusionTexture;
#endif

// Текстурные координаты. По 2 float-а на вершину. Без разрывов
#if TEXCOORD_COUNT > 0
  layout (set = STATIC, binding = 9) readonly buffer Texcoord0Buffer
  {
    float data[];
  } TEXCOORD_0;
#endif
