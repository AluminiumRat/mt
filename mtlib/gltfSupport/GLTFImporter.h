#pragma once

#include <gltfSupport/BaseGLTFImporter.h>
#include <hld/meshDrawable/MeshDrawable.h>

namespace mt
{
  class TechniqueManager;
  class TechniqueResource;

  class GLTFImporter : public BaseGLTFImporter
  {
  public:
    using DrawablesList = std::vector<std::unique_ptr<MeshDrawable>>;

  public:
    GLTFImporter( CommandQueueGraphic& uploadingQueue,
                  TextureManager& textureManager,
                  TechniqueManager& techniqueManager,
                  LoadingPolicy resourcesLoadingPolicy);
    GLTFImporter(const GLTFImporter&) = delete;
    GLTFImporter& operator = (const GLTFImporter&) = delete;
    virtual ~GLTFImporter() noexcept;

    DrawablesList importGLTF(const std::filesystem::path& file);

  private:
    //  Набор hld ассетов для одного gltf меша
    using MeshAssets = std::vector<ConstRef<MeshAsset>>;
    //  Набор маш асетов для всей сцены. Индекс в векторе соответствует индексу
    //  меша в tinygltf::Model
    using AssetLib = std::vector<MeshAssets>;

  private:
    void _clear() noexcept;

    //  Рекурсивный обход нод в иерархии gltf модели
    void _processNode(int nodeIndex, const glm::mat4& parentTransform);
    //  Создать MeshDrawable, когда при обходе иерархии gltf наткнулись на меш
    void _processMesh(int gltfMeshIndex, const glm::mat4& tansform);

    //  Получить уже созданный или создать новый набор ассетов для меша
    //  meshIndex соответствует индексу меша в tinygltf::Model
    const MeshAssets& _getAsset(int meshIndex);
    //  Создать набор меш ассетов для меша из gltf
    MeshAssets _createMeshAssets(int meshIndex);

    //  Подключить к ассету и настроить техники рисования
    //  Возвращает false, если по какой-то причине не удалось создать техники
    bool _adjustAsset(MeshAsset& targetAsset,
                      const GPUVerticesData& vertices,
                      const GLTFMaterial& material,
                      const DataBuffer& gpuMaterial,
                      const std::string& meshName);

  private:
    TechniqueManager& _techniqueManager;
    LoadingPolicy _resourcesLoadingPolicy;

    //  Подменная белая текстура
    ConstRef<TechniqueResource> _whiteTexture;

    const tinygltf::Model* _model;
    CommandProducerGraphic* _producer;

    AssetLib _meshAssets;

    DrawablesList _drawables;
  };
}