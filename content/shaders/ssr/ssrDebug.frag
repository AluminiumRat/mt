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

  //  Смещение точек от края ячейки HiZ, чтобы попадать в следующую ячейку
  vec2 pointShift = directionSign.xy * commonData.hiZExtent.zw * 0.5f;
  //  С каким краем клеток HiZ будем искать пересечение
  vec2 cellBorder = max(directionSign.xy, 0.0f);

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

  //  Шаг при контрэйсинге
  float coneStep = (1 + boundleFactor.x) / (1 - boundleFactor.x);

  //  Подсвечиваем пучек, в котором будем собирать отражение
  float myBundleRad = myT * boundleFactor.x;
  if(distToRay <= myBundleRad) outColor.a = 0.1f;

  float minTStep = max(commonData.hiZExtent.z, commonData.hiZExtent.w);
  float currentT = 2.0f * minTStep;
  vec3 currentPoint = startPoint + currentT * direction;

  uint stepCount = 0;
  vec4 color = vec4(0);
  while(++stepCount <= 50 && color.a < 1.0f)
  {
    vec4 checkpointColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);

    float boundleRadiusSS = boundleFactor.x * currentT;
    float checkpointMip = log2(boundleRadiusSS * commonData.hiZExtent.x) + 1;
    vec3 hizValue = textureLod( sampler2D(hiZBuffer,
                                          commonLinearSamplerClamped),
                                currentPoint.xy,
                                checkpointMip).xyz;

    float boundleRadiusWorld = boundleFactor.y * currentT;
    float linearDepth = hyperbolicToLinearDepth(currentPoint.z);
    if(linearDepth + boundleRadiusWorld > hizValue.x)
    {
      checkpointColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // Подсвечиваем чекпоинт
    if(length(texCoord - currentPoint.xy) < 0.002f) addColor(checkpointColor);

    // Следующий чекпоинт
    float nextT = currentT * coneStep;
    nextT = max(nextT, currentT + minTStep);
    currentPoint = startPoint + direction * nextT;
    currentT = nextT;
  }
}