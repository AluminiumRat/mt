#ifndef GLTF_MATERIAL_H
#define GLTF_MATERIAL_H

//  Настройки материала, загруженные из gltf файла
//  c++ часть лежит в gltfSupport/GLTFMaterial.h
struct GLTFMaterial
{
  //  Режим смешивания альфы, сюда пишутся значения GLTFMaterial::AlphaMode
  //    OPAQUE_ALPHA_MODE = 0
  //    MASK_ALPHA_MODE = 1
  //    BLEND_ALPHA_MODE = 2
  int alphaMode;
  //  Граница для обрезания по альфе в режиме MASK_ALPHA_MODE
  float alphaCutoff;
  
  int doubleSided; // 0 = false, 1 = true
  vec4 baseColor;
  vec3 emission;
  float metallic;
  float roughness;
  float normalTextureScale;
  float occlusionTextureStrength;
};

#endif