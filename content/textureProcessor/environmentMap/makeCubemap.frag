#version 450
//  Шейдер переводит карту окружения из равнопромежуточной проекции(spheremap)
//  в cubemap-у

layout (set = STATIC) uniform IntrinsicData
{
  uint mipLevel;
  uint arrayIndex;
} intrinsic;

layout (set = STATIC) uniform texture2D colorTexture;
layout (set = STATIC) uniform sampler colorSampler;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

//  Матрицы для поворота векторов "вперед/вверх/враво" из координат стороны
//  кубмапы в мировые координаты
const mat3 sideRotate[] = { mat3( 0,  0,  1,
                                  0, -1,  0,
                                  1,  0,  0),

                            mat3( 0,  0, -1,
                                  0, -1,  0,
                                 -1,  0,  0),

                            mat3( 1,  0,  0,
                                  0,  0, -1,
                                  0,  1,  0),

                            mat3( 1,  0,  0,
                                  0,  0,  1,
                                  0, -1,  0),

                            mat3( 1,  0,  0,
                                  0, -1,  0,
                                  0,  0, -1),

                            mat3(-1,  0,  0,
                                  0, -1,  0,
                                  0,  0,  1)};

void main()
{
  //  Получаем направление сэмплирования в координатах
  //  текущей обрабатываемой стороны кубмапы
  vec2 planePosition = texCoord * 2.0f - vec2(1.0f);
  vec3 frontDirection = normalize(vec3(planePosition, 1.f));

  //  Переводим направление сэмплирования в мировые коодиныты
  uint side = intrinsic.arrayIndex;
  frontDirection = sideRotate[side] * frontDirection;

  //  Переводим направление сэмплирования в широту/долготу, а из них
  //  в текстурные координаты сферамапы
  float lon = 0;
  if(frontDirection.x != 0.0f || frontDirection.y != 0.0f)
  {
    float lonCos = frontDirection.x / length(frontDirection.xy);
    lon = acos(lonCos);
    if(frontDirection.y < 0) lon = 2.0f * M_PI - lon;
  }
  float uCoord = 1.0f - lon / (2.0f * M_PI);

  float latSin = frontDirection.z;
  float lat = asin(latSin);
  float vCoord = 0.5f - lat / M_PI;

  outColor = texture( sampler2D(colorTexture, colorSampler),
                      vec2(uCoord, vCoord));
}