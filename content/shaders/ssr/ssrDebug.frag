#version 450

#include "ssr/buildSSR.inl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

//  Альфа блендинг в outColor
void addColor(vec4 color)
{
  outColor.rgb = mix(outColor.rgb, color.rgb, color.a);
  outColor.a += (1.0f - outColor.a) * color.a;
}

void main()
{
  outColor = vec4(0.0f, 0.0f, 0.0f, 0.5f);

  vec2 ssCoords = debugParams.mousePos;
  if(length(texCoord - ssCoords) < 0.002f) addColor(vec4(1.0f, 1.0f, 1.0f, 0.5f));

  float linearDepth = texture(sampler2D(linearDepthHalfBuffer,
                                        commonNearestSamplerClamped),
                              ssCoords).r;
  //  Попали в небо
  if(linearDepth == LINEAR_DEPTH_EMPTY_VALUE) return;

  //  Луч, по которому будет проходить маршинг
  vec3 startPoint, direction;
  getRay(startPoint, direction, ssCoords, linearDepth);
  vec3 directionSign = sign(direction);

  float maxT = getMaxT(startPoint, direction, directionSign);

  //  Подсвечиваем луч
  vec2 directionNorm = vec2(direction.y, -direction.x);
  vec2 fromStartPoint = texCoord - startPoint.xy;
  float distToRay = abs(dot(fromStartPoint, directionNorm));
  float myT = dot(fromStartPoint, direction.xy);
  if(distToRay < 0.002f && myT > 0.0f && myT <= maxT) addColor(vec4(1.0f));

  vec2 boundleFactor = getBoundleFactor(startPoint, direction, maxT);
  if(boundleFactor.x >= 1.0f)
  {
    // Слишком широкий пучек
    addColor(vec4(1.0f, 0.0f, 0.0f, 0.5f));
    return;
  }

  //  Шаг при конетрэйсинге
  float coneStep = (1 + boundleFactor.x) / (1 - boundleFactor.x);

  //  Подсвечиваем пучек, в котором будем собирать отражение
  float myBundleRad = myT * boundleFactor.x;
  if(distToRay <= myBundleRad) outColor.a = 0.1f;

  //  Смещение точек от края ячейки HiZ, чтобы попадать в следующую ячейку
  vec2 pointShift = directionSign.xy * commonData.hiZExtent.zw * 0.5f;
  //  С каким краем клеток HiZ будем искать пересечение
  vec2 cellBorder = max(directionSign.xy, 0.0f);

  float minTStep = max(commonData.hiZExtent.z, commonData.hiZExtent.w);
  vec4 currentPoint = vec4( startPoint + 2 * minTStep * direction,
                            2 * minTStep);

  uint stepCount = 0;
  while(++stepCount <= 50)
  {
    // Подсвечиваем чекпоинт
    if(length(texCoord - currentPoint.xy) < 0.002f) addColor(vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Следующий чекпоинт
    float nextT = currentPoint.w * coneStep;
    nextT = max(nextT, currentPoint.w + minTStep);
    currentPoint = vec4(startPoint + direction * nextT, nextT);
  }
}