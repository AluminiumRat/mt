#include <cstring>
#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <gltfSupport/BaseGLTFImporter.h>
#include <resourceManagement/TextureManager.h>
#include <util/Assert.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <vkr/queue/CommandQueueGraphic.h>

namespace fs = std::filesystem;

using namespace mt;

//  Перевод из координат glfw в движковые. Просто замена осей, чтобы
//  направление вверх было +Z
const glm::mat4 BaseGLTFImporter::glfwToMTTransform(1, 0, 0, 0,
                                                    0, 0, 1, 0,
                                                    0,-1, 0, 0,
                                                    0, 0, 0, 1);

BaseGLTFImporter::BaseGLTFImporter( CommandQueueGraphic& uploadingQueue,
                                    TextureManager* textureManager,
                                    LoadingPolicy textureLoadingPolicy) :
  _uploadingQueue(uploadingQueue),
  _device(uploadingQueue.device()),
  _textureManager(textureManager),
  _textureLoadingPolicy(textureLoadingPolicy)
{
}

BaseGLTFImporter::~BaseGLTFImporter() noexcept = default;

void BaseGLTFImporter::_clear() noexcept
{
  _file.clear();
  _baseDir.clear();
  _filename.clear();
  _model.reset();
  _textures.clear();
  _materials.clear();
  _gpuMaterials.clear();
}

const tinygltf::Model& BaseGLTFImporter::parseFile(
                                              const std::filesystem::path& file)
{
  _clear();

  _file = file;
  _baseDir = file.parent_path();
  _filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& fileLoader = ContentLoader::getLoader();
  std::vector<char> fileData = fileLoader.loadData(file);

  _model.reset(new tinygltf::Model);

  tinygltf::TinyGLTF gltfLoader;
  _prepareLoader(gltfLoader);

  std::string error;
  std::string warning;
  bool success = gltfLoader.LoadASCIIFromString(
                                          _model.get(),
                                          &error,
                                          &warning,
                                          (const char*)fileData.data(),
                                          (unsigned int)fileData.size(),
                                          pathToUtf8(_baseDir));
  if(!error.empty()) throw std::runtime_error(_filename + " : " + error);
  if(!success) throw std::runtime_error(_filename + " : unable to parse file");
  if(!warning.empty()) Log::warning() << _filename << " : " << warning;

  _textures.resize(_model->textures.size());
  _materials.resize(_model->materials.size());
  _gpuMaterials.resize(_model->materials.size());

  return *_model;
}

void BaseGLTFImporter::_prepareLoader(tinygltf::TinyGLTF& loader)
{
  //  Подключаем свой fileLoader к tinygltf через FsCallbacks
  ContentLoader& fileLoader = ContentLoader::getLoader();

  tinygltf::FsCallbacks fsCallbacks{};

  fsCallbacks.FileExists =
    [&](const std::string filename, void*)
    {
      return fileLoader.exists(utf8ToPath(filename));
    };

  fsCallbacks.ExpandFilePath =
    [](const std::string filename, void*)
    {
      return filename;
    };

  fsCallbacks.ReadWholeFile =
    [&](std::vector<unsigned char>* target,
        std::string*,
        const std::string filename,
        void*)
    {
      MT_ASSERT(target != nullptr);
      std::vector<char> data = fileLoader.loadData(utf8ToPath(filename));
      if(data.empty()) return true;
      target->resize(data.size());
      memcpy(target->data(), data.data(), data.size());
      return true;
    };

  fsCallbacks.WriteWholeFile =
    []( std::string*,
        const std::string&,
        const std::vector<unsigned char>&,
        void*)
    {
      return false;
    };

  fsCallbacks.GetFileSizeInBytes =
    [](size_t* filesize_out, std::string*, const std::string&, void*)
    {
      //  Этот калбэк нужен только для проверки на максимальный размер, поэтому
      //  просто вернем 0, чтобы всегда проходить проверку
      *filesize_out = 0;
      return true;
    };

  loader.SetFsCallbacks(fsCallbacks);
}

const TechniqueResource*
                BaseGLTFImporter::getTexture(const tinygltf::TextureInfo& info)
{
  int textureIndex = info.index;
  if(textureIndex == -1) return nullptr;
  MT_ASSERT(textureIndex < _textures.size());

  if(_textures[textureIndex] != nullptr) return _textures[textureIndex].get();

  ConstRef<TechniqueResource> newTexture = _loadTexture(textureIndex);
  _textures[textureIndex] = newTexture;

  return newTexture.get();
}

ConstRef<TechniqueResource>
                          BaseGLTFImporter::_loadTexture(int textureIndex) const
{
  if(_textureManager == nullptr)
  {
    return ConstRef<TechniqueResource>(new TechniqueResource);
  }

  const tinygltf::Texture& gltfTexture = _model->textures[textureIndex];
  if(gltfTexture.source < 0)
  {
    throw std::runtime_error(_filename + ": texture " + std::to_string(textureIndex) + " doesn't have source");
  }

  tinygltf::Image image = _model->images[gltfTexture.source];

  if(image.uri.empty())
  {
    throw std::runtime_error(_filename + ": image " + std::to_string(gltfTexture.source) + " has empty uri");
  }
  fs::path imagePath = _baseDir / utf8ToPath(image.uri);

  if(_textureLoadingPolicy == LOAD_ASYNC)
  {
    return _textureManager->scheduleLoading(imagePath,
                                            _uploadingQueue,
                                            true);
  }
  else
  {
    return _textureManager->loadImmediately(imagePath,
                                            _uploadingQueue,
                                            true);
  }
}

const GLTFMaterial& BaseGLTFImporter::getMaterial(int materialIndex)
{
  if(_materials[materialIndex] != nullptr) return *_materials[materialIndex];

  std::unique_ptr<GLTFMaterial> newMaterial = _loadMaterial(materialIndex);
  _materials[materialIndex] = std::move(newMaterial);

  return *_materials[materialIndex];
}

std::unique_ptr<GLTFMaterial> BaseGLTFImporter::_loadMaterial(int materialIndex)
{
  std::unique_ptr<GLTFMaterial> material(new GLTFMaterial);
  *material = GLTFMaterial{};

  const tinygltf::Material& gltfMaterial = _model->materials[materialIndex];

  // Режим смешивания по альфе
  if(gltfMaterial.alphaMode == "OPAQUE")
  {
    material->alphaMode = GLTFMaterial::OPAQUE_ALPHA_MODE;
  }
  else if(gltfMaterial.alphaMode == "MASK")
  {
    material->alphaMode = GLTFMaterial::MASK_ALPHA_MODE;
  }
  else if(gltfMaterial.alphaMode == "BLEND")
  {
    material->alphaMode = GLTFMaterial::BLEND_ALPHA_MODE;
  }
  else
  {
    material->alphaMode = GLTFMaterial::OPAQUE_ALPHA_MODE;
    Log::warning() << _filename << " : material: " << gltfMaterial.name << ": unknown alphaMode: " << gltfMaterial.alphaMode;
  }
  material->alphaCutoff = (float)gltfMaterial.alphaCutoff;

  // Базовые параметры
  material->doubleSided = gltfMaterial.doubleSided;
  material->baseColor = glm::vec4(
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
  material->emission = glm::vec3( gltfMaterial.emissiveFactor[0],
                                  gltfMaterial.emissiveFactor[1],
                                  gltfMaterial.emissiveFactor[2]);
  material->metallic = (float)gltfMaterial.pbrMetallicRoughness.metallicFactor;
  material->roughness =
                      (float)gltfMaterial.pbrMetallicRoughness.roughnessFactor;
  material->normalTextureScale = (float)gltfMaterial.normalTexture.scale;
  material->occlusionTextureStrength =
                                  (float)gltfMaterial.occlusionTexture.strength;

  // Грузим текстуры
  material->baseColorTexture = getTexture(
                            gltfMaterial.pbrMetallicRoughness.baseColorTexture);
  material->baseColorTexCoord =
                    gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord;

  material->metallicRoughnessTexture =
        getTexture(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture);
  material->metallicRoughnessTexCoord =
            gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

  material->normalTexture =
          getTexture((const tinygltf::TextureInfo&)gltfMaterial.normalTexture);
  material->normalTexCoord = gltfMaterial.normalTexture.texCoord;

  material->occlusionTexture = getTexture(
                  (const tinygltf::TextureInfo&)gltfMaterial.occlusionTexture);
  material->occlusionTexCoord = gltfMaterial.occlusionTexture.texCoord;

  material->emissiveTexture = getTexture(gltfMaterial.emissiveTexture);
  material->emissiveTexCoord = gltfMaterial.emissiveTexture.texCoord;

  return material;
}

const DataBuffer& BaseGLTFImporter::getGPUMaterial(
                                              int materialIndex,
                                              CommandProducerGraphic& producer)
{
  //  Проверяем, может материал уже был слздан ранее
  if(_gpuMaterials[materialIndex] != nullptr)
  {
    return *_gpuMaterials[materialIndex];
  }

  std::string bufferName =
                        _filename + ":" + _model->materials[materialIndex].name;

  //  Создаем буфер из CPU материала
  const GLTFMaterial& material = getMaterial(materialIndex);
  GLTFMaterialGPU gpuData{};
  gpuData.alphaMode = material.alphaMode;
  gpuData.alphaCutoff = material.alphaCutoff;
  gpuData.doubleSided = material.doubleSided ? 1 : 0;
  gpuData.baseColor = material.baseColor;
  gpuData.emission = material.emission;
  gpuData.metallic = material.metallic;
  gpuData.roughness = material.roughness;
  gpuData.normalTextureScale = material.normalTextureScale;
  gpuData.occlusionTextureStrength = material.occlusionTextureStrength;
  gpuData.baseColorTexCoord = material.baseColorTexCoord;
  gpuData.metallicRoughnessTexCoord = material.metallicRoughnessTexCoord;
  gpuData.normalTexCoord = material.normalTexCoord;
  gpuData.occlusionTexCoord = material.occlusionTexCoord;
  gpuData.emissiveTexCoord = material.emissiveTexCoord;
  ConstRef<DataBuffer> newGpuMaterial = uploadData( &gpuData,
                                                    sizeof(gpuData),
                                                    producer,
                                                    bufferName);
  _gpuMaterials[materialIndex] = newGpuMaterial;
  return *newGpuMaterial;
}

ConstRef<DataBuffer> BaseGLTFImporter::uploadData(
                                                const void* data,
                                                size_t dataSize,
                                                CommandProducerGraphic& producer,
                                                const std::string& debugName)
{
  Device& device = producer.queue().device();
  ConstRef<DataBuffer> gpuBuffer(new DataBuffer(
                                            device,
                                            dataSize,
                                            DataBuffer::STORAGE_BUFFER,
                                            debugName.c_str()));
  Ref<DataBuffer> stagingBuffer(new DataBuffer( device,
                                                dataSize,
                                                DataBuffer::UPLOADING_BUFFER,
                                                "Uploading buffer"));
  stagingBuffer->uploadData(data, 0, dataSize);
  producer.copyFromBufferToBuffer(*stagingBuffer, *gpuBuffer, 0, 0, dataSize);
  return gpuBuffer;
}

BaseGLTFImporter::VerticesData
                BaseGLTFImporter::getVerticesData(
                                          const tinygltf::Primitive& primitive,
                                          const std::string& meshName) const
{
  VerticesData vertices;
  //  Обходим все атрибуты вершин и считываем нужные
  for(std::map<std::string, int>::const_iterator iAttribute =
                                                primitive.attributes.begin();
      iAttribute != primitive.attributes.end();
      iAttribute++)
  {
    const std::string& attributeName = iAttribute->first;
    const tinygltf::Accessor& accessor = _model->accessors[iAttribute->second];
    _processVertexAttribute(attributeName, accessor, vertices, meshName);
  }

  //  Считываем индексы вершин, если есть
  if(primitive.indices >= 0)
  {
    const tinygltf::Accessor& accessor = _model->accessors[primitive.indices];
    vertices.indices = _readIndices(accessor, meshName);
    vertices.vertexCount = (uint32_t)accessor.count;
  }

  return vertices;
}

void BaseGLTFImporter::_processVertexAttribute(
                                            const std::string& attributeName,
                                            const tinygltf::Accessor& accessor,
                                            VerticesData& targetData,
                                            const std::string& meshName) const
{
  //  Проверяем количество вершин
  if(targetData.vertexCount == 0)
  {
    targetData.vertexCount = (uint32_t)accessor.count;
  }
  else
  {
    if(targetData.vertexCount != (uint32_t)accessor.count) throw std::runtime_error(meshName + ": has different vertices counts in vertex attributes");
  }

  //  Загружаем данные в нужный контейнер
  if(attributeName == "POSITION")
  {
    targetData.positions = _readAccessorData<glm::vec3>(accessor,
                                                        attributeName);
    //  По позициям определяем ограничивающий AABB
    if(accessor.minValues.size() == 3 || accessor.maxValues.size() == 3)
    {
      targetData.bound = AABB((float)accessor.minValues[0],
                              (float)accessor.minValues[1],
                              (float)accessor.minValues[2],
                              (float)accessor.maxValues[0],
                              (float)accessor.maxValues[1],
                              (float)accessor.maxValues[2]);
    }
    else Log::warning() << meshName << ":" << attributeName << "doesen't have correct minimal and maximal values";
  }
  else if(attributeName == "NORMAL")
  {
    targetData.normals = _readAccessorData<glm::vec3>(accessor,
                                                      attributeName);
  }
  else if(attributeName == "TANGENT")
  {
    targetData.tangents = _readAccessorData<glm::vec4>( accessor,
                                                        attributeName);
  }
  else if(attributeName == "TEXCOORD_0")
  {
    targetData.texCoord0 = _readAccessorData<glm::vec2>(accessor,
                                                        attributeName);
  }
  else if(attributeName == "TEXCOORD_1")
  {
    targetData.texCoord1 = _readAccessorData<glm::vec2>(accessor,
                                                        attributeName);
  }
  else if(attributeName == "TEXCOORD_2")
  {
    targetData.texCoord2 = _readAccessorData<glm::vec2>(accessor,
                                                        attributeName);
  }
  else if(attributeName == "TEXCOORD_3")
  {
    targetData.texCoord3 = _readAccessorData<glm::vec2>(accessor,
                                                        attributeName);
  }
}

template<typename ComponentType>
std::vector<ComponentType> BaseGLTFImporter::_readAccessorData(
                                        const tinygltf::Accessor& accessor,
                                        const std::string& componentName) const
{
  //  Проверяем, что мы собираемся прочитать корректные данные
  int32_t componentSize =
            tinygltf::GetComponentSizeInBytes(accessor.componentType) *
                                tinygltf::GetNumComponentsInType(accessor.type);
  if(componentSize != sizeof(ComponentType)) throw std::runtime_error(componentName + ": wrong component size");

  //  Ищем место, откуда будем считывать данные
  size_t dataStartOffset = accessor.byteOffset;
  if(accessor.bufferView < 0) throw std::runtime_error(_filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                                      _model->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(_filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = _model->buffers[bufferview.buffer];

  //  Копируем данные из tinyGltf буфера
  std::vector<ComponentType> resultData;
  resultData.resize(accessor.count);
  if(bufferview.byteStride == componentSize || bufferview.byteStride == 0)
  {
    //  В буфере нет разрывов между данными, можем копировать одним куском
    size_t dataSize = componentSize * accessor.count;
    memcpy(resultData.data(), &buffer.data[dataStartOffset], dataSize);
  }
  else
  {
    //  В буфере есть разрывы, собираем данные по кусочкам
    std::byte* dstCursor = (std::byte*)resultData.data();
    const unsigned char* srcCursor = &buffer.data[dataStartOffset];
    for(size_t i = 0 ; i < accessor.count; i++)
    {
      memcpy(dstCursor, srcCursor, componentSize);
      dstCursor += componentSize;
      srcCursor += bufferview.byteStride;
    }
  }

  return resultData;
}

std::vector<uint32_t> BaseGLTFImporter::_readIndices(
                                            const tinygltf::Accessor& accessor,
                                            const std::string& meshName) const
{
  //  Всегда используем 32-х разрядные индексы
  size_t indexSize = 4;

  //  Проверяем, что индексы из gltf файла можно уложить в 32 разряда
  int32_t srcIndexSize =
                      tinygltf::GetComponentSizeInBytes(accessor.componentType);
  if(srcIndexSize > indexSize) throw std::runtime_error(meshName + ":INDICES: too big index type");

  //  Ищем место, откуда будем копировать данные
  size_t dataStartOffset = accessor.byteOffset;
  if(accessor.bufferView < 0) throw std::runtime_error(_filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                                      _model->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(_filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = _model->buffers[bufferview.buffer];

  //  Копируем данные
  std::vector<uint32_t> indices;
  indices.resize(accessor.count, 0);
  std::byte* dstCursor = (std::byte*)indices.data();
  const unsigned char* srcCursor = &buffer.data[dataStartOffset];
  size_t srcStride = bufferview.byteStride == 0 ? srcIndexSize :
                                                  bufferview.byteStride;
  for(size_t i = 0 ; i < accessor.count; i++)
  {
    memcpy(dstCursor, srcCursor, srcIndexSize);
    dstCursor += indexSize;
    srcCursor += srcStride;
  }

  return indices;
}

BaseGLTFImporter::GPUVerticesData
            BaseGLTFImporter::uploadVertices( const VerticesData& data,
                                              CommandProducerGraphic& producer,
                                              const std::string& meshName) const
{
  //  Префикс в именах буферов
  std::string namePrefix = meshName.empty() ? "" : meshName + ":";

  GPUVerticesData gpuData{};
  gpuData.vertexCount = data.vertexCount;
  gpuData.bound = data.bound;

  gpuData.indices = _uploadBuffer(data.indices,
                                  producer,
                                  namePrefix + "INDICES");

  gpuData.positions = _uploadBuffer(data.positions,
                                    producer,
                                    namePrefix + "POSITION");

  gpuData.normals = _uploadBuffer(data.normals,
                                  producer,
                                  namePrefix + "NORMAL");

  gpuData.tangents = _uploadBuffer( data.tangents,
                                    producer,
                                    namePrefix + "TANGENT");

  gpuData.texCoord0 = _uploadBuffer(data.texCoord0,
                                    producer,
                                    namePrefix + "TEXCOORD_0");

  gpuData.texCoord1 = _uploadBuffer(data.texCoord1,
                                    producer,
                                    namePrefix + "TEXCOORD_1");

  gpuData.texCoord2 = _uploadBuffer(data.texCoord2,
                                    producer,
                                    namePrefix + "TEXCOORD_2");

  gpuData.texCoord3 = _uploadBuffer(data.texCoord3,
                                    producer,
                                    namePrefix + "TEXCOORD_3");
  return gpuData;
}

template<typename ComponentType>
ConstRef<DataBuffer> BaseGLTFImporter::_uploadBuffer(
                                        const std::vector<ComponentType>& data,
                                        CommandProducerGraphic& producer,
                                        const std::string& bufferName) const
{
  if(data.empty()) return ConstRef<DataBuffer>();
  return uploadData(data.data(),
                    data.size() * sizeof(ComponentType),
                    producer,
                    bufferName);
}

glm::mat4 BaseGLTFImporter::getTransform(const tinygltf::Node& node)
{
  if(!node.matrix.empty())
  {
    return glm::mat4(
              node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
              node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
              node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
              node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
  }
  else
  {
    glm::mat4 transform(1);

    if(!node.translation.empty())
    {
      transform = glm::translate( glm::mat4(1),
                                  glm::vec3(node.translation[0],
                                            node.translation[1],
                                            node.translation[2]));
    }
    if(!node.rotation.empty())
    {
      glm::quat rotation( (float)node.rotation[3],
                          (float)node.rotation[0],
                          (float)node.rotation[1],
                          (float)node.rotation[2]);

      transform *= glm::toMat4(rotation);
    }
    if(!node.scale.empty())
    {
      transform *= glm::scale(glm::mat4(1),
                              glm::vec3(node.scale[0],
                                        node.scale[1],
                                        node.scale[2]));
    }
    return transform;
  }
}
