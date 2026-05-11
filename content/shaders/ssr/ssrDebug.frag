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
                                        commonNearestSampler),
                              ssCoords).r;
  //  Попали в небо
  if(linearDepth == LINEAR_DEPTH_EMPTY_VALUE) return;

  //  Луч, по которому будет проходить маршинг
  vec3 startPoint, direction;
  getRay(startPoint, direction, ssCoords, linearDepth);
  vec3 directionSign = sign(direction);

  //  Подсвечиваем луч
  vec2 directionNorm = vec2(direction.y, -direction.x);
  vec2 fromStartPoint = texCoord - startPoint.xy;
  float distToRay = abs(dot(fromStartPoint, directionNorm));
  float myT = dot(fromStartPoint, direction.xy);
  if(distToRay < 0.002f && myT > 0.0f) addColor(vec4(1.0f));

  float maxT = getMaxT(startPoint, direction, directionSign);

  float boundleFactor = getBoundleFactor(startPoint, direction, maxT);

  //  Подсвечиваем пучек, по которому будем собирать отражение
  float myBundleRad = myT * boundleFactor;
  if(distToRay <= myBundleRad) outColor.a = 0.1f;
}