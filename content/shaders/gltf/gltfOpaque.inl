#include <lib/commonSet.inl>
#include <gltf/gltfMaterial.inl>

#define NORMALTEXTURE_OFF 0
#define NORMALTEXTURE_VERTEX_TANGENT 1
#define NORMAL_TEXTURE_FRAGMENT_TANGENT 2

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
#if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
  layout (set = STATIC, binding = 4) readonly buffer TangentBuffer
  {
    vec4 data[];
  } TANGENT;
#endif

#if TEXCOORD_COUNT > 0
  layout (set = STATIC, binding = 5) uniform texture2D baseColorTexture;

  layout (set = STATIC,
          binding = 6) uniform texture2D metallicRougghnessTexture;

  layout (set = STATIC, binding = 7) uniform texture2D occlusionTexture;
#endif

#if NORMALTEXTURE_MODE != NORMALTEXTURE_OFF
  layout (set = STATIC, binding = 8) uniform texture2D normalTexture;
#endif

#ifdef EMISSIVETEXTURE_ENABLED
  layout (set = STATIC, binding = 9) uniform texture2D emissiveTexture;
#endif

// Текстурные координаты. По 2 float-а на вершину. Без разрывов
// Текстурных координат может быть 0, 1 или 4
#if TEXCOORD_COUNT > 0
  layout (set = STATIC, binding = 10) readonly buffer Texcoord0Buffer
  {
    float data[];
  } TEXCOORD_0;

  #if TEXCOORD_COUNT > 1
    layout (set = STATIC, binding = 11) readonly buffer Texcoord1Buffer
    {
      float data[];
    } TEXCOORD_1;

    layout (set = STATIC, binding = 12) readonly buffer Texcoord2Buffer
    {
      float data[];
    } TEXCOORD_2;
    layout (set = STATIC, binding = 13) readonly buffer Texcoord3Buffer
    {
      float data[];
    } TEXCOORD_3;
  #endif
#endif
