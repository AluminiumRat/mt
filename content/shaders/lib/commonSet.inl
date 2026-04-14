#ifndef COMMON_SET_INL
#define COMMON_SET_INL

#include <lib/cameraData.inl>
#include <lib/environmentData.inl>
#include "lib/octahedronEncoding.inl"

//  Магическое число, если оно встречается в линейном буфере глубины, значит
//  туда не было отрендерено никакой геометрии. Т.е это фон
#define LINEAR_DEPTH_EMPTY_VALUE 10000.0f

layout (set = COMMON, binding = 0) uniform CommonData
{
  //  Информация о положении камеры на текущем кадре
  CameraData cameraData;
  //  Информация о положении камеры на предыдущем кадре
  CameraData prevCameraData;

  EnvironmentData environment;

  //  Размер области, в которую отрисовывается конечная картинка
  //  zw = 1 / xy
  vec4 frameBufferExtent;
  ivec2 iFrameBufferExtent;
  //  Размер половинных буферов
  //  zw = 1 / xy
  vec4 halfFrameExtent;
  ivec2 iHalfFrameExtent;
  //  Размер нулевого мипа для HiZ(иерархический depth буффер)
  //  zw = 1 / xy
  vec4 hiZExtent;
  ivec2 iHiZExtent;
  uint hiZMipCount;

  //  Номер кадра, начиная с 0. Относится только к конкретному ColorFrameBuilder
  int frameIndex;
} commonData;

//  LUT текстура для image based lighting и сэмплер для её чтения
layout (set = COMMON,
        binding = 1) uniform texture2D iblLut;
layout (set = COMMON,
        binding = 2) uniform sampler iblLutSampler;

//  Префильтрованная Irradiance текстура для IBL освещения
layout (set = COMMON,
        binding = 3) uniform textureCube iblIrradianceMap;

//  Префильтрованная specular текстура для IBL освещения
layout (set = COMMON,
        binding = 4) uniform textureCube iblSpecularMap;

//  Линейный буфер глубины в половинном разрешении
layout (set = COMMON,
        binding = 5) uniform texture2D linearDepthHalfBuffer;

//  Буфер нормалей в половинном разрешении
layout (set = COMMON,
        binding = 6) uniform texture2D normalHalfBuffer;

//  HiZ. Иерархический depth буффер. Размерность - половинное разрешение
//  округленное вниз до степени двойки. Можно получить из
//  commonData.hiZExtent и commonData.iHiZExtent
//  x компонента - минимальное расстояние
//  y компонента - максимальное расстояние
layout (set = COMMON,
        binding = 7) uniform texture2D hiZBuffer;

//  Буфер репроекции из текущего кадра в предыдущий. В половинном разрешении.
//  XY - смещение скрин спэйс координат точки
//  Z - ожидаемое смещение в линейном буфере глубины
//  W - достоверность репроецкии (1 - уверены, что репроекция
//      корректная, 0 - уверены, что репроекция не корректная, то есть точка
//      не имеет истории на экране и появилась только в этом фрэйме)
layout (set = COMMON,
        binding = 8) uniform texture2D reprojectionBuffer;

//  Скринспейсовый буфер с тенями
layout (set = COMMON,
        binding = 9) uniform texture2D shadowBuffer;

//  Дефолтный сэмплер общего назначения.
//  Линейная фильтрация, VK_SAMPLER_ADDRESS_MODE_REPEAT, анизотропия 4
layout (set = COMMON,
        binding = 10) uniform sampler commonLinearSampler;

//  Дефолтный сэмплер общего назначения.
//  Nearest фильтрация
layout (set = COMMON,
        binding = 11) uniform sampler commonNearestSampler;

//  Получить размер пикселя для половинных буферов на определенной дистанции
//  от камеры
float getHalfBufferPixelSize(float linearDepth)
{
  return commonData.cameraData.fovY * linearDepth *
                                                  commonData.halfFrameExtent.w;
}

//  Прочитать нормаль из normalHalfBuffer
//  PixelCoord - координаты пикселя в normalHalfBuffer
vec3 getNormalFromHalfBuffer(ivec2 pixelCoord)
{
  return octahedronDecode(texelFetch(sampler2D( normalHalfBuffer,
                                                commonNearestSampler),
                                     pixelCoord,
                                     0).xy);
}

//  Вектор, специально предназначенный для вычисления мировых координат
//  по линейному буферу глубины. Представляет из себя напрвление "из камеры",
//  но с нормализованной во вью пространстве координатой z
//  ssCoords - экранные координаты в диапазоне [0; 1]
vec3 getPosRestoreVec(vec2 ssCoords)
{
  return commonData.cameraData.leftTopRPV +
            ssCoords.x * commonData.cameraData.leftToRightRPV +
            ssCoords.y * commonData.cameraData.topToBottomRPV;
}

//  Получить направление "из камеры"(в мировых координатах) по экранным
//    координатам
//  ssCoords - экранные координаты в диапазоне [0; 1]
vec3 getViewDirection(vec2 ssCoords)
{
  return normalize(getPosRestoreVec(ssCoords));
}

//  Восстановить позицию в мировых координатах по экранным координатам и
//    линейной глубине
//  ssCoords - экранные координаты в диапазоне [0; 1]
vec3 getWorldPosition(vec2 ssCoords, float linearDepth)
{
  vec3 posRestoreVec = getPosRestoreVec(ssCoords);
  return commonData.cameraData.eyePoint + posRestoreVec * linearDepth;
}

#endif