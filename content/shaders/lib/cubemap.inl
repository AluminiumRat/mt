#ifndef CUBEMAP_INL
#define CUBEMAP_INL

//  Матрицы для поворота векторов "вперед/вверх/враво" из координат стороны
//  кубмапы в мировые координаты
const mat3 cubemapSideRotate[] = {  mat3( 0,  0,  1,
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

//  Получить вектор-направление по текстурным координатам на какой-либо стороне
//  кубемапы
vec3 cubemapDirection(vec2 texcoords, uint side)
{
  //  Получаем направление сэмплирования в координатах
  //  текущей обрабатываемой стороны кубмапы
  vec2 planePosition = texcoords * 2.0f - vec2(1.0f);
  vec3 direction = normalize(vec3(planePosition, 1.f));

  //  Переводим направление сэмплирования в мировые коодиныты
  return cubemapSideRotate[side] * direction;
}

#endif