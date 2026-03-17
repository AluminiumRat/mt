//  Октогедронное кодирование нормалей
//  https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
#ifndef OCTAHEDRON_ENCODING_INL
#define OCTAHEDRON_ENCODING_INL

//  Вспомогательная функция, используется в octahedronEncode
vec2 octahedronWrap(vec2 v)
{
  return (1.0f - abs(v.yx)) * sign(v.xy);
}

//  Переводим нормаль в квадрат [-1;1]
vec2 octahedronEncode(vec3 n)
{
  n /= (abs(n.x) + abs(n.y) + abs(n.z));
  n.xy = n.z >= 0.0 ? n.xy : octahedronWrap(n.xy);
  return n.xy;
}

//  Восстанавливаем нормаль из квадрата [-1;1]
vec3 octahedronDecode(vec2 f)
{
  // https://twitter.com/Stubbesaurus/status/937994790553227264
  vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
  float t = -1.0f * clamp(-n.z, 0.0f, 1.0f);
  n.xy += sign(n.xy) * t;
  return normalize(n);
}

#endif