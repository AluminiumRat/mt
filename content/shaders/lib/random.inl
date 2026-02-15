#ifndef RANDOM_INL
#define RANDOM_INL

//  Based on http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

//  Hammersley Point Set
//  Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 hammersley2d(uint i, uint N, uint seed)
{
	uint bits = ((i + seed) << 16u) | ((i + seed) >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

//  Равномерный сэмплинг по полусфере
//  samplerValue - равномерно распределенная по [0,1] величина
//  https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
vec3 sampleHemisphere(vec2 samplerValue)
{
  float z = samplerValue.x;
  float circleRadius = sqrt(1.0f - z * z);
  float phi = 2.0f * M_PI * samplerValue.y;
  return vec3(circleRadius * cos(phi), circleRadius * sin(phi), z);
}

//  Равномерный сэмплинг по диску радиуса 1
//  samplerValue - равномерно распределенная по [0,1] величина
//  https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
vec2 concentricSampleDisk(vec2 samplerValue)
{
  vec2 offset = 2.0f * samplerValue - vec2(1.0f);
  if(offset.x == 0.0f && offset.y == 0.0f) return vec2(0.0f);

  float theta;
  float radius;
  if(abs(offset.x) > abs(offset.y))
  {
    radius = offset.x;
    theta = M_PI / 4.0f * (offset.y / offset.x);
  }
  else
  {
    radius = offset.y;
    theta = M_PI / 2.0f - M_PI / 4.0f * (offset.x / offset.y);
  }

  return radius * vec2(cos(theta), sin(theta));
}

//  Косинусно-взвешенный сэмплинг по полусфере радиуса 1
//  samplerValue - равномерно распределенная по [0,1] величина
//  https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
vec3 cosineSampleHemisphere(vec2 samplerValue)
{
  vec2 projection = concentricSampleDisk(samplerValue);
  float z = sqrt( max(0.0f,
                      1.0f - dot(projection, projection)));
  return vec3(projection, z);
}

#endif