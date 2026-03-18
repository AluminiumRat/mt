// Библиотека с общим кодом для фрагментных шейдеров рендера gltf моделей
#ifndef GLTF_FRAG_COMMONINL
#define GLTF_FRAG_COMMONINL

#include "gltf/gltfInput.inl"
#include "lib/color.inl"

layout(location = 0) in vec3 inNormal;
//  xyz - положение в мировых координатах; w - линейная глубина
layout(location = 1) in vec4 inWorldPosition;
//  Тангент и бинормаль используются только для текстур нормалей
#if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
  layout(location = 2) in vec3 inTangent;
  layout(location = 3) in vec3 inBinormal;
#endif

//  Текстурные координаты для извлечения данных из полноэкранных
//  буферов
layout(location = 4) noperspective in vec2 inSSCoords;

#if TEXCOORD_COUNT == 1
  layout(location = 5) in vec2 inTexCoord;
#endif
#if TEXCOORD_COUNT > 1
  layout(location = 5) in vec2 inTexCoord[4];
#endif

//  Получить текстурные координаты по их индексу
#if TEXCOORD_COUNT > 0
vec2 getTexCoord(int index)
{
  #if TEXCOORD_COUNT == 1
    return inTexCoord;
  #else
    return inTexCoord[index];
  #endif
}
#endif

//  Нормаль к поверхности
vec3 getNormal()
{
  vec3 normal = normalize(inNormal);
  #if NORMALTEXTURE_MODE != NORMALTEXTURE_OFF && TEXCOORD_COUNT > 0
    vec3 textureNormal = texture(
                      sampler2D(normalTexture, commonLinearSampler),
                      getTexCoord(materialBuffer.material.normalTexCoord)).rgb;
    //  Считанное значение прошло в сэмплере через sRGB->linear преобразование.
    //  Но исходные значения и так были в линейном пространстве, поэтому
    //  переводим обратно
    textureNormal = linearToSrgbFast(textureNormal);
    textureNormal = textureNormal * 2.0f - 1.0f;

    //  Вычисляем матрицу tbn
    #if NORMALTEXTURE_MODE == NORMALTEXTURE_VERTEX_TANGENT
      vec3 tangent = normalize(inTangent);
      vec3 binormal = normalize(inBinormal);
    #else
      //  NORMAL_TEXTURE_FRAGMENT_TANGENT
      vec3 tangent = normalize(
                            dFdx(inWorldPosition.xyz) * dFdx(getTexCoord(0).x) +
                            dFdy(inWorldPosition.xyz) * dFdy(getTexCoord(0).x));
      vec3 binormal = -normalize(cross(normal, tangent));
    #endif
    mat3 tbn = mat3(tangent, binormal, normal);

    //  Применяем tbn к нормали из текстуры и смешиваем с вершинной нормалью
    normal = mix( normal,
                  tbn * textureNormal,
                  materialBuffer.material.normalTextureScale);
    normal = normalize(normal);
  #endif

  return normal;
}

#endif