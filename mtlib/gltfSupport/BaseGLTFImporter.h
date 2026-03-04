#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <gltfSupport/GLTFMaterial.h>
#include <util/AABB.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace tinygltf
{
  struct Accessor;
  class Model;
  class Node;
  struct Primitive;
  struct TextureInfo;
  class TinyGLTF;
}

namespace mt
{
  class CommandProducerGraphic;
  class CommandQueueGraphic;
  class Device;
  class TextureManager;

  //  Базовый функционал для создания конкретных импортеров из GLTF файлов
  class BaseGLTFImporter
  {
  public:
    enum LoadingPolicy
    {
      LOAD_ASYNC,
      LOAD_IMMEDIATELY
    };

  public:
    //  Перевод из координат glfw в движковые. Просто замена осей, чтобы
    //  направление вверх было +Z
    static const glm::mat4 glfwToMTTransform;

  public:
    //  textureManager - менеджер, через который будет производиться
    //    загрузка текстур. Если nullptr, то загрузка производиться не будет,
    //    ресурсы, соответствующие текстурам будут пустыми
    //  textureLoadingPolicy - способ загрузки текстур, если LOAD_ASYNC, то
    //    загрузка текстур будет отправлена в асинхронную очередь менеджера
    //    текстур, а getTexture будет возвращать ресурс с дефолтной текстурой.
    //    Если LOAD_IMMEDIATELY, то текстуры будет загружаться немедленно, как
    //    только будут запрошены.
    BaseGLTFImporter( CommandQueueGraphic& uploadingQueue,
                      TextureManager* textureManager,
                      LoadingPolicy textureLoadingPolicy);
    BaseGLTFImporter(const BaseGLTFImporter&) = delete;
    BaseGLTFImporter& operator = (const BaseGLTFImporter&) = delete;
    virtual ~BaseGLTFImporter() noexcept;

    inline CommandQueueGraphic& uploadingQueue() const noexcept;
    inline Device& device() const noexcept;
    inline TextureManager* textureManager() const noexcept;

  protected:
    //  Обнаруженные вершинные данные меша
    struct VerticesData
    {
      //  Сколько всего вершин в меше
      uint32_t vertexCount = 0;
      AABB bound;

      std::vector<uint32_t> indices;

      std::vector<glm::vec3> positions;
      std::vector<glm::vec3> normals;
      //  xyz - вектор тангента, w - знак(+-1), указывающий на "рукость"
      //  tbn базиса
      std::vector<glm::vec4> tangents;
      std::vector<glm::vec2> texCoord0;
      std::vector<glm::vec2> texCoord1;
      std::vector<glm::vec2> texCoord2;
      std::vector<glm::vec2> texCoord3;

      VerticesData() = default;
      VerticesData(const VerticesData&) = default;
      VerticesData(VerticesData&&) = default;
      VerticesData& operator = (const VerticesData&) = default;
      VerticesData& operator = (VerticesData&&) = default;
      ~VerticesData() noexcept = default;
    };

    //  Информация об обнаруженных вертексных буферах меша
    struct GPUVerticesData
    {
      //  Сколько всего вершин в меше
      uint32_t vertexCount = 0;

      AABB bound;
      ConstRef<DataBuffer> indices;
      ConstRef<DataBuffer> positions;
      ConstRef<DataBuffer> normals;
      ConstRef<DataBuffer> tangents;
      ConstRef<DataBuffer> texCoord0;
      ConstRef<DataBuffer> texCoord1;
      ConstRef<DataBuffer> texCoord2;
      ConstRef<DataBuffer> texCoord3;
    };

  protected:
    //  Предварительный парсинг файла перед основной работой импортера
    const tinygltf::Model& parseFile(const std::filesystem::path& file);

    //  Файл, который был распарсен с помощью parseFile
    inline const std::filesystem::path& file() const noexcept;
    //  Имя файла в utf8. Используется для лога и дебага
    inline const std::string& filename() const noexcept;
    //  Папка, в которой находится распарсенный файл. Используется для
    //  поиска текстур
    inline const std::filesystem::path& baseDir() const noexcept;

    //  Хэлпер. Просто загрузить какие-то данные в storage буфер на ГПУ
    //  producer используется для загрузки данных на GPU. То есть фактически
    //    данные будут записаны на GPU только после сабмита в очередь команд
    //  debugName - имя, которое будет выставлено буферу
    static ConstRef<DataBuffer> uploadData( const void* data,
                                            size_t dataSize,
                                            CommandProducerGraphic& producer,
                                            const std::string& debugName);

    //  Получить текстуру, соответствующую info
    //  Может вернуть пустой ресурс, если импортеру не передан мэнеджер текстур
    //  Если textureLoadingPolicy == LOAD_ASYNC, то может вернуть ресурс с
    //    дефолтной текстурой, при этом загрузка текстуры начнется в
    //    параллельном потоке.
    //  Если info.index == -1 (т.е. info не указывает ни на какую текстуру), то
    //    возвращает nullptr
    const TechniqueResource* getTexture(const tinygltf::TextureInfo& info);

    //  Получить описание материала по индексу материала из tinygltf::Model
    const GLTFMaterial& getMaterial(int materialIndex);

    //  Получить DataBuffer с записанной в нем структурой GLTFMaterialGPU
    //  materialIndex соответствует индексу материала из tinygltf::Model
    //  producer используется для загрузки данных на GPU. То есть фактически
    //    данные будут записаны на GPU только после сабмита в очередь команд
    const DataBuffer& getGPUMaterial( int materialIndex,
                                      CommandProducerGraphic& producer);

    //  meshName нужен для лога, можно оставить пустым
    VerticesData getVerticesData( const tinygltf::Primitive& primitive,
                                  const std::string& meshName) const;

    //  Загрузить данные вершин на GPU
    //  producer используется для загрузки данных на GPU. То есть фактически
    //    данные будут записаны на GPU только после сабмита в очередь команд
    //  meshName используется для дебаг целей. Можно оставить пустым.
    GPUVerticesData uploadVertices( const VerticesData& data,
                                    CommandProducerGraphic& producer,
                                    const std::string& meshName) const;

    //  Получить матрицу трансформации для node
    //  Внимание!!! Возвращает матрицу трансформации из локальных координат
    //    node в родительскую систему координаты (вышестоящая нода). Для того
    //    чтобы получить трансформацию в мировую систему координат надо пройтись
    //    по всей иерархии нод
    static glm::mat4 getTransform(const tinygltf::Node& node);

  private:
    //  Очистить внутреннее состояние перед импортом файла
    void _clear() noexcept;

    //  Настройить лодер tinygltf перед парсингом файла
    static void _prepareLoader(tinygltf::TinyGLTF& loader);

    ConstRef<TechniqueResource> _loadTexture(int textureIndex) const;
    std::unique_ptr<GLTFMaterial> _loadMaterial(int materialIndex);

    //  Обработать вершинный аттрибут и внести результаты в targetData
    void _processVertexAttribute( const std::string& attributeName,
                                  const tinygltf::Accessor& accessor,
                                  VerticesData& targetData,
                                  const std::string& meshName) const;
    template<typename ComponentType>
    std::vector<ComponentType> _readAccessorData(
                                        const tinygltf::Accessor& accessor,
                                        const std::string& componentName) const;

    std::vector<uint32_t> _readIndices( const tinygltf::Accessor& accessor,
                                        const std::string& meshName) const;

    //  Загрузить вершинный буффер на GPU.
    //  Если data пустой, возвращает nullptr
    template<typename ComponentType>
    ConstRef<DataBuffer> _uploadBuffer( const std::vector<ComponentType>& data,
                                        CommandProducerGraphic& producer,
                                        const std::string& bufferName) const;
  private:
    CommandQueueGraphic& _uploadingQueue;
    Device& _device;
    TextureManager* _textureManager;
    LoadingPolicy _textureLoadingPolicy;

    std::filesystem::path _file;
    std::filesystem::path _baseDir;
    std::string _filename;
    std::unique_ptr<tinygltf::Model> _model;

    //  Ресурсы для подключения текстур к техникам. Индексы соответствуют
    //  индексам текстур в tinygltf::Model::textures
    using Textures = std::vector<ConstRef<TechniqueResource>>;
    Textures _textures;

    //  Информация по всем созданным материалам gltf сцены. Индекс в векторе
    //  соответствует индексу материала в tinygltf::Model::materials
    using Materials = std::vector<std::unique_ptr<GLTFMaterial>>;
    Materials _materials;

    //  Структуры GLTFMaterialGPU, загруженные в буферы на GPU
    //  Индекс в векторе соответствует индексу материала в
    //    tinygltf::Model::materials
    using GPUMaterials = std::vector<ConstRef<DataBuffer>>;
    GPUMaterials _gpuMaterials;
  };

  inline CommandQueueGraphic& BaseGLTFImporter::uploadingQueue() const noexcept
  {
    return _uploadingQueue;
  }

  inline Device& BaseGLTFImporter::device() const noexcept
  {
    return _device;
  }

  inline TextureManager* BaseGLTFImporter::textureManager() const noexcept
  {
    return _textureManager;
  }

  inline const std::filesystem::path& BaseGLTFImporter::file() const noexcept
  {
    return _file;
  }

  inline const std::string& BaseGLTFImporter::filename() const noexcept
  {
    return _filename;
  }

  inline const std::filesystem::path& BaseGLTFImporter::baseDir() const noexcept
  {
    return _baseDir;
  }
}