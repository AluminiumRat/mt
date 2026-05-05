#ifndef BUILD_SSR_INL
#define BUILD_SSR_INL
//  Общие данные и методы при построении SSR

#include "lib/commonSet.inl"

layout( r11f_g11f_b10f,
        set = STATIC,
        binding = 0) uniform image2D outReflectionBuffer;

layout( set = STATIC,
        binding = 1) uniform texture2D prevHDR;
        
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
  vec3 normal = getNormalFromHalfBuffer(ivec2(gl_GlobalInvocationID.xy));

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

//  Координаты ячейки HiZ по скринспэйс координатам
ivec2 getCell(vec2 ssCoords, int mipLevel)
{
  ivec2 pixelCoord = ivec2(ssCoords * commonData.hiZExtent.xy);
  return pixelCoord >> mipLevel;
}

//  Пересечение с границей ячейки
float cellIntersection( vec2 startPoint,
                        vec2 direction,
                        ivec2 cell,
                        int hizMip,
                        vec2 border)
{
  // Размер ячейки в screenSpace координатах
  vec2 cellSize = vec2(1 << hizMip);
  cellSize *= commonData.hiZExtent.zw;

  // Вертикаль и горизонталь, с которыми надо пересечься
  vec2 cellBorders = (cell + border) * cellSize;
  cellBorders = min(cellBorders, 1.0f);

  vec2 ortoDist = (cellBorders - startPoint) / direction;
  return min(ortoDist.x, ortoDist.y);
}

//  Получить максимальное значение t, до которого нужно
//  маршить
float getMaxT(vec3 startPoint, vec3 direction, vec3 directionSign)
{
  //  Края clip пространства в направлении луча
  vec3 border = max(directionSign, 0.0f);
  vec3 ortoDist = (border - startPoint) / direction;
  return min(min(ortoDist.x, ortoDist.y), ortoDist.z);
}

//  Получить первую точку на луче для маршинга
vec3 getFirstPoint( vec3 startPoint,
                    vec3 direction,
                    vec2 cellBorder,
                    vec2 pointShift)
{
  ivec2 cell = getCell(startPoint.xy, 0);
  float tValue = cellIntersection(startPoint.xy,
                                  direction.xy,
                                  cell,
                                  0,
                                  cellBorder);
  vec3 firstPoint = startPoint + direction * tValue;
  // Небольшое смещение, чтобы уйти за границу ячейки HiZ
  firstPoint.xy += pointShift;
  return firstPoint;
}

#endif