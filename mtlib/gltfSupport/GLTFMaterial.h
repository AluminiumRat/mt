#pragma once

#include <glm/glm.hpp>

#include <technique/TechniqueResource.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  //  Данные материала, хранящиеся в буфере на GPU
  struct GLTFMaterialGPU
  {
    //  Режим смешивания альфы, сюда пишутся значения GLTFMaterial::AlphaMode
    alignas(4) uint32_t alphaMode;
    alignas(4) float alphaCutoff;
    // 0 = false, 1 = true
    alignas(4) uint32_t doubleSided;
    alignas(16) glm::vec4 baseColor;
    alignas(16) glm::vec3 emission;
    alignas(4) float metallic;
    alignas(4) float roughness;
    alignas(4) float normalTextureScale;
    alignas(4) float occlusionTextureStrength;
  };

  struct GLTFMaterial
  {
    //  Режим смешивания по альфе
    enum AlphaMode
    {
      OPAQUE_ALPHA_MODE = 0,
      MASK_ALPHA_MODE = 1,
      BLEND_ALPHA_MODE = 2
    };
    AlphaMode alphaMode;
    //  Граница для обрезания по альфе в режиме MASK_ALPHA_MODE
    float alphaCutoff;

    bool doubleSided;

    glm::vec4 baseColor;
    glm::vec3 emission;
    float metallic;
    float roughness;
    float normalTextureScale;
    float occlusionTextureStrength;

    ConstRef<TechniqueResource> baseColorTexture;
    ConstRef<TechniqueResource> metallicRoughnessTexture;
    ConstRef<TechniqueResource> normalTexture;
    ConstRef<TechniqueResource> occlusionTexture;
    ConstRef<TechniqueResource> emissiveTexture;

    // storage буфер, в котором находится GLTFMaterialGPU
    ConstRef<DataBuffer> materialData;
  };
}