#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <hld/meshDrawable/MeshDrawable.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace tinygltf
{
  struct Accessor;
  class Model;
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
    };

  private:
    void _clear();
    //  Основной алгоритм загрузки без обвязки на исключения
    void _import(const std::filesystem::path& file);
    //  Пропарсить сырые данные из файла и заполнить tinygltf::Model
    void _parseGLTF(const std::vector<char>& fileData,
                    tinygltf::Model& model,
                    const char* baseDir);
    //  Настройить лодер tinygltf перед парсингом файла
    void _prepareLoader(tinygltf::TinyGLTF& loader) const;
    //  Просто загрузить какие-то данные в storage буфер на ГПУ
    static ConstRef<DataBuffer> _uploadData(const void* data,
                                            size_t dataSize,
                                            CommandProducerGraphic& producer,
                                            const std::string& debugName);
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
    void _attachTechniques( MeshAsset& targetAsset,
                            VerticesInfo& verticesInfo,
                            const std::string& meshName);

    //  Рекурсивный обход нод в иерархии gltf модели
    void _processNode(int nodeIndex);
    //  Создать MeshDrawable, когда при обходе иерархии gltf наткнулись на меш
    void _processMesh(int gltfMeshIndex);

  private:
    CommandQueueGraphic& _uploadingQueue;
    Device& _device;
    CommandProducerGraphic* _producer;
    TextureManager& _textureManager;
    TechniqueManager& _techniqueManager;

    DrawablesList _drawables;

    std::string _filename;
    const tinygltf::Model* _gltfModel;
    AssetLib _meshAssets;
    glm::mat4 _currentTansform;
  };
}