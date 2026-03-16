#pragma once

#include <gltfSupport/BaseGLTFImporter.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <vkr/accelerationStructure/BLASInstance.h>

namespace mt
{
  class TechniqueManager;
  class TechniqueResource;

  class GLTFImporter : public BaseGLTFImporter
  {
  public:
    using DrawablesList = std::vector<std::unique_ptr<MeshDrawable>>;
    using BLASList = std::vector<std::unique_ptr<BLASInstance>>;
    struct Results
    {
      DrawablesList drawables;
      BLASList blases;

      Results() = default;
      Results(Results&&) = default;
      Results& operator = (Results&&) = default;
    };

  public:
    GLTFImporter( CommandQueueGraphic& uploadingQueue,
                  TextureManager& textureManager,
                  TechniqueManager& techniqueManager,
                  LoadingPolicy resourcesLoadingPolicy,
                  bool createBLAS);
    GLTFImporter(const GLTFImporter&) = delete;
    GLTFImporter& operator = (const GLTFImporter&) = delete;
    virtual ~GLTFImporter() noexcept;

    Results importGLTF(const std::filesystem::path& file);

  protected:
    virtual ConstRef<DataBuffer> createIndexBuffer(
                                        size_t dataSize,
                                        const char* bufferName) const override;
    virtual ConstRef<DataBuffer> createVertexBuffer(
                                        size_t dataSize,
                                        const char* bufferName) const override;


  private:
    //  Набор hld ассетов для одного gltf меша
    using MeshAssets = std::vector<ConstRef<MeshAsset>>;
    //  Набор меш асетов для всей сцены. Индекс в векторе соответствует индексу
    //  меша в tinygltf::Model
    using AssetLib = std::vector<MeshAssets>;

    //  Набор BLAS асетов для всей сцены. Индекс в векторе соответствует индексу
    //  меша в tinygltf::Model
    using BLASLib = std::vector<ConstRef<BLAS>>;

  private:
    void _clear() noexcept;

    //  Рекурсивный обход нод в иерархии gltf модели
    void _processNode(int nodeIndex, const glm::mat4& parentTransform);
    //  Создать MeshDrawable, когда при обходе иерархии gltf наткнулись на меш
    void _processMesh(int gltfMeshIndex, const glm::mat4& tansform);

    //  Получить уже созданный или создать новый набор ассетов для меша
    //  meshIndex соответствует индексу меша в tinygltf::Model
    const MeshAssets& _getMeshAsset(int meshIndex);
    //  Создать набор меш ассетов для меша из gltf
    MeshAssets _createMeshAssets(int meshIndex);

    //  Подключить к ассету и настроить техники рисования
    //  Возвращает false, если по какой-то причине не удалось создать техники
    bool _adjustMeshAsset(MeshAsset& targetAsset,
                          const GPUVerticesData& vertices,
                          const GLTFMaterial& material,
                          const DataBuffer& gpuMaterial,
                          const std::string& meshName);

    //  Получить уже созданный или создать новый BLAS для меша
    //  meshIndex соответствует индексу меша в tinygltf::Model
    const BLAS* _getBLAS(int meshIndex);

  private:
    TechniqueManager& _techniqueManager;
    LoadingPolicy _resourcesLoadingPolicy;
    bool _createBLAS;

    //  Подменная белая текстура
    ConstRef<TechniqueResource> _whiteTexture;

    const tinygltf::Model* _model;
    CommandProducerGraphic* _producer;

    AssetLib _meshAssets;
    BLASLib _blasAssets;

    DrawablesList _drawables;
    BLASList _blases;
  };
}