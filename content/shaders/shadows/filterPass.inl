//  Общий код для горизонтального и вертикального проходов фильтрации
//  результатов рэй трэйсинга теней

#include <shadows/rayQueryShadows.inl>

layout (constant_id = 0) const int GROUP_SIZE = 1024;

//  Кэш для хранения временных результатов
shared float valueCache[GROUP_SIZE];
shared float depthCache[GROUP_SIZE];
shared vec3 normalCache[GROUP_SIZE];

const float gaussKernel[5] = float[](0.54f, 0.86f, 1.0f,  0.86f, 0.54f);

//  Добавить к value и weight значения из кэша одного из соседей текущего
//  обрабатываемого фильтра. Часть алгоритма spatial фильтрации
void applyNeighbor( int neighborIndex,
                    int neighborIndexShift,
                    inout float value,
                    inout float weight,
                    float depth,
                    float depthTreshold,
                    vec3 normal)
{
  neighborIndex = clamp(neighborIndex, 0, GROUP_SIZE - 1);

  //  Фильтр Гауса
  float neighborWeight = gaussKernel[neighborIndexShift + 2];

  //  Взвешивание по глубине
  float neighborDepth = depthCache[neighborIndex];
  float depthDelta = abs(depth - neighborDepth);
  neighborWeight *= max((depthTreshold - depthDelta) / depthTreshold, 0.0f);

  //  Взвешивание по нормали
  vec3 neighborNormal = normalCache[neighborIndex];
  neighborWeight *= max(dot(neighborNormal, normal), 0.0f);

  value += valueCache[neighborIndex] * neighborWeight;
  weight += neighborWeight;
}
