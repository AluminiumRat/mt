//  Общий код для горизонтального и вертикального проходов фильтрации
//  результатов рэй трэйсинга теней

#include <shadows/rayQueryShadows.inl>

const float gaussKernel[5] = float[](0.54f, 0.86f, 1.0f,  0.86f, 0.54f);

//  Максимальное отклонение при взвешивании сэмплов по глубине
float getDepthTreshold( float depth,
                        vec3 normal)
{
  float pixelSize = getHalfBufferPixelSize(depth);
  float normalFactor = 1.0f - abs(dot(normal,
                                      commonData.cameraData.frontVector));
  return (5.0f + 3.0f * normalFactor) * pixelSize;
}

float getWeight(ivec2 sampleCoord,
                int pixelShift,
                float depth,
                float depthTreshold,
                vec3 normal)
{
  //  Фильтр Гауса
  float weight = gaussKernel[pixelShift + 2];

  //  Взвешивание по глубине
  float currentDepth = texelFetch(sampler2D(linearDepthHalfBuffer,
                                            commonNearestSampler),
                                  sampleCoord,
                                  0).x;
  float depthDelta = abs(depth - currentDepth);
  weight *= max((depthTreshold - depthDelta) / depthTreshold, 0.0f);

  //  Взвешивание по нормали
  vec3 currentNormal = octahedronDecode(
                                    texelFetch( sampler2D( normalHalfBuffer,
                                                          commonNearestSampler),
                                                sampleCoord,
                                                0).xy);
  weight *= max(dot(currentNormal, normal), 0.0f);

  return weight;
}
