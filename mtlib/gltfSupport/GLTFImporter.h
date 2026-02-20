#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <gltfSupport/GLTFMaterial.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace tinygltf
{
  struct Accessor;
  class Model;
  class Node;
  struct TextureInfo;
  class TinyGLTF;
}

namespace mt
{
  class CommandProducerGraphic;
  class CommandQueueGraphic;
  class Device;
  class TechniqueManager;
  class TextureManager;

  class GLTFImporter
  {
  public:
    using DrawablesList = std::vector<std::unique_ptr<MeshDrawable>>;

  public:
    GLTFImporter( CommandQueueGraphic& uploadingQueue,
                  TextureManager& textureManager,
                  TechniqueManager& techniqueManager);
    GLTFImporter(const GLTFImporter&) = delete;
    GLTFImporter& operator = (const GLTFImporter&) = delete;
    virtual ~GLTFImporter() noexcept = default;

    DrawablesList importGLTF(const std::filesystem::path& file);

  private:
    //  Набор hld ассетов для одного gltf меша
    using MeshAssets = std::vector<ConstRef<MeshAsset>>;
    //  Набор маш асетов для всей сцены. Индекс в векторе соответствует индексу
    //  меша в tinygltf::Model
    using AssetLib = std::vector<MeshAssets>;

    //  Информация об обнаруженных вертексных буферах меша
    struct VerticesInfo
    {
      //  Сколько всего вершин в меше
      uint32_t vertexCount = 0;
      bool positionFound = false;
      bool normalFound = false;
      bool indicesFound = false;
      bool texcoord0Found = false;
      bool tangentFound = false;
    };

    //  Информация по всем материалам gltf сцены. Индекс в векторе соответствует
    //  индексу материала в tinygltf::Model::materials
    using Materials = std::vector<GLTFMaterial>;

    //  Ресурсы для подключения текстур к техникам. Индексы соответствуют
    //  индексам текстур в tinygltf::Model::textures
    using Textures = std::vector<ConstRef<TechniqueResource>>;

  private:
    void _clear();
    //  Основной алгоритм загрузки без обвязки на исключения
    void _import(const std::filesystem::path& file);
    //  Пропарсить сырые данные из файла и заполнить tinygltf::Model
    void _parseGLTF(const std::vector<char>& fileData, tinygltf::Model& model);
    //  Настройить лодер tinygltf перед парсингом файла
    void _prepareLoader(tinygltf::TinyGLTF& loader) const;
    //  Просто загрузить какие-то данные в storage буфер на ГПУ
    static ConstRef<DataBuffer> _uploadData(const void* data,
                                            size_t dataSize,
                                            CommandProducerGraphic& producer,
                                            const std::string& debugName);
    //  Обработать информацию о текстуре, отправить команду на её загрузку и
    //  заполнить _textures
    void _createTexture(int textureIndex);

    //  Обработать настройки gltf материала и транслировать их в собственные
    void _createMaterialInfo(int materialIndex);
    //  Загрузить данные материала на GPU
    ConstRef<DataBuffer> _createGPUMaterialInfo(
                                          const GLTFMaterial& material,
                                          const std::string& bufferName) const;
    //  Получить текстуру из уже загруженных.
    //  MaterialName и textureName нужны только для лога
    ConstRef<TechniqueResource> _getTexture(
                                        const tinygltf::TextureInfo& info,
                                        const char* materialName,
                                        const char* textureName) const;
    //  Создать на GPU буфер с данными для gltf аксессора (общий случай)
    ConstRef<DataBuffer> _createAccessorBuffer(
                                            const tinygltf::Accessor& accessor,
                                            const std::string& debugName) const;
    //  Специализированный метод для загрузки индекс буффера из glyf аксессора
    ConstRef<DataBuffer> _createIndexBuffer(const tinygltf::Accessor& accessor,
                                            const std::string& debugName) const;
    //  Создать набор меш ассетов для меша из gltf и вставить их в _meshAssets
    void _createMeshAssets(int meshIndex);
    //  Обработать отдельный вертекс аттрибут, добавить его в targetAsset и
    //  обновить vertexInfo
    void _processVertexAttribute( const std::string& attributeName,
                                  const tinygltf::Accessor& accessor,
                                  MeshAsset& targetAsset,
                                  VerticesInfo& verticesInfo,
                                  const std::string& meshName);
    //  Подключить к ассету и настроить техники рисования
    //  Возвращает false, если по какой-то причине не удалось создать техники
    bool _attachTechniques( MeshAsset& targetAsset,
                            const VerticesInfo& verticesInfo,
                            const GLTFMaterial& material,
                            const std::string& meshName);

    //  Рекурсивный обход нод в иерархии gltf модели
    void _processNode(int nodeIndex);
    //  Получить матрицу трансформации из node и добавить её к _currentTansform
    void _applyNodeTransform(const tinygltf::Node& node);
    //  Создать MeshDrawable, когда при обходе иерархии gltf наткнулись на меш
    void _processMesh(int gltfMeshIndex);

  private:
    CommandQueueGraphic& _uploadingQueue;
    Device& _device;
    CommandProducerGraphic* _producer;
    TextureManager& _textureManager;
    TechniqueManager& _techniqueManager;

    DrawablesList _drawables;

    std::filesystem::path _file;
    std::filesystem::path _baseDir;
    std::string _filename;
    const tinygltf::Model* _gltfModel;

    Textures _textures;
    Materials _materials;
    AssetLib _meshAssets;
    glm::mat4 _currentTansform;
  };
}