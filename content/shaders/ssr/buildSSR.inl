#ifndef BUILD_SSR_INL
#define BUILD_SSR_INL
//  Общие данные и методы при построении SSR

#include "lib/commonSet.inl"

layout( r11f_g11f_b10f,
        set = STATIC,
        binding = 0) uniform image2D outReflectionBuffer;

layout( set = STATIC,
        binding = 1) uniform texture2D prevHDR;

layout( set = STATIC,
        binding = 2) uniform texture1D angleDistribTexture;

layout(push_constant) uniform DebugParams
{
  vec2 mousePos;
} debugParams;

float hyperbolicToLinearDepth(float hyperbolic)
{
  float near = commonData.cameraData.nearDistance;
  float far = commonData.cameraData.farDistance;
  return far * near / (hyperbolic * (far - near) + near);
}

float linearToHiperbolicDepth(float linear)
{
  float near = commonData.cameraData.nearDistance;
  float far = commonData.cameraData.farDistance;
  return near * (far - linear) / linear / (far - near);
}

//  Вычислить луч, вдоль которого будет происходить маршинг
//  Резултат: origin - начало луча, direction - направление. Для каждого из них:
//    xy - скрин спэйс координаты [0,1]
//    z - гиперболическая глубина
void getRay(out vec3 origin,
            out vec3 direction,
            vec2 ssCoords,
            float linearDepth)
{
  vec3 worldPos = getWorldPosition(ssCoords, linearDepth);
  vec3 normal = getNormalFromHalfBuffer(ssCoords);

  //  Делаем небольшое смещение вдоль нормали, чтобы на старте маршинга
  //  не получить ложное пересечение
  worldPos += normal * 2.0f * getHalfBufferPixelSize(linearDepth);

  //  Переводим начало луча в экранные координаты
  vec4 clipSpacePos =
                commonData.cameraData.viewProjectionMatrix * vec4(worldPos, 1);
  clipSpacePos.xyz /= clipSpacePos.w;
  origin = vec3(clipSpacePos.xy * 0.5f + 0.5f, clipSpacePos.z);

  //  Вычисляем направление луча. Находим какую-нибудь точку на луче отражения,
  //  переводим в скрин спэйс и вычитаем начало луча
  vec3 viewDir = getViewDirection(ssCoords);
  vec3 reflectionDir = reflect(viewDir, normal);
  vec3 shiftedPoint = worldPos + reflectionDir;
  vec4 shiftedPointSS =
            commonData.cameraData.viewProjectionMatrix * vec4(shiftedPoint, 1);
  shiftedPointSS.xyz /= shiftedPointSS.w;
  shiftedPointSS.xy = shiftedPointSS.xy * 0.5f + 0.5f;
  direction = shiftedPointSS.xyz - origin;
  direction = direction / length(direction.xy);

  //  Немного смещаем направление, чтобы избежать нулей (и деление на ноль)
  direction += step(vec3(0.0f), direction) * 0.0001f;
}

//  Получить максимальное значение t, до которого нужно
//  маршить
float getMaxT(vec3 startPoint, vec3 direction, vec3 directionSign)
{
  //  Края SS пространства в направлении луча
  vec3 border = max(directionSign, 0.0f);
  vec3 ortoDist = (border - startPoint) / direction;
  return min(min(ortoDist.x, ortoDist.y), ortoDist.z);
}

//  Получить мгодитель для радиуса пучка который будем интегрировать от параметра t луча
//    по которому маршим
//  startPoint, direction - луч, по которому проводим маршинг (в ss координатах и
//    гиперболическим z)
//  maxT - максимальное значение t для луча при маршинге
float getBoundleFactor( vec3 startPoint,
                        vec3 direction,
                        float maxT)
{
  float rougness = textureLod(sampler2D(roughnessHalfBuffer, commonLinearSampler),
                              startPoint.xy,
                              0).r;
  float bundleTg = textureLod(sampler1D(angleDistribTexture, commonLinearSampler),
                              rougness,
                              0).r;
  //float bundleTg = 0.1f;

  //  Начало и конец луча во вью координатах
  vec3 finalPoint = startPoint + direction * maxT;
  vec4 startPointView = commonData.cameraData.invProjectionMatrix *
                            vec4(startPoint.xy * 2.0f - 1.0f, startPoint.z, 1);
  startPointView /= startPointView.w;
  vec4 finalPointView = commonData.cameraData.invProjectionMatrix *
                            vec4(finalPoint.xy * 2.0f - 1.0f, finalPoint.z, 1);
  finalPointView /= finalPointView.w;
  float rayLength = length(finalPointView.xyz - startPointView.xyz);

  //  Радиус пучка (вью координаты) в конечной точке луча
  float boundleRadius = rayLength * bundleTg;
  float finalPointLinearDepth = hyperbolicToLinearDepth(finalPoint.z);

  //  Радиус пучка в SS координатах
  float boundleSSRadius = boundleRadius /
                                    getScreenSizeWorld(finalPointLinearDepth).x;

  return boundleSSRadius / maxT;
}

#endif