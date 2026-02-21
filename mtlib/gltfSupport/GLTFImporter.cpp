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

#include <gltfSupport/GLTFImporter.h>
#include <resourceManagement/TechniqueManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <vkr/queue/CommandQueueGraphic.h>

namespace fs = std::filesystem;

using namespace mt;

//  Перевод из координат glfw в движковые. Просто замена осей, чтобы
//  направление вверх было +Z
constexpr glm::mat4 glfwToMTTransform(1, 0, 0, 0,
                                      0, 0, 1, 0,
                                      0,-1, 0, 0,
                                      0, 0, 0, 1);

GLTFImporter::GLTFImporter( CommandQueueGraphic& uploadingQueue,
                            TextureManager& textureManager,
                            TechniqueManager& techniqueManager) :
  _uploadingQueue(uploadingQueue),
  _device(uploadingQueue.device()),
  _producer(nullptr),
  _textureManager(textureManager),
  _techniqueManager(techniqueManager),
  _gltfModel(nullptr),
  _currentTansform(glfwToMTTransform)
{
}

void GLTFImporter::_clear()
{
  _producer = nullptr;
  _drawables.clear();
  _file.clear();
  _baseDir.clear();
  _filename.clear();
  _gltfModel = nullptr;
  _textures.clear();
  _materials.clear();
  _meshAssets.clear();
  _currentTansform = glfwToMTTransform;
}

GLTFImporter::DrawablesList GLTFImporter::importGLTF(
                                              const std::filesystem::path& file)
{
  try
  {
    _import(file);
  }
  catch(...)
  {
    _clear();
    throw;
  }

  DrawablesList result = std::move(_drawables);
  _clear();
  return result;
}

void GLTFImporter::_import(const std::filesystem::path& file)
{
  _file = file;
  _baseDir = file.parent_path();
  _filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& fileLoader = ContentLoader::getLoader();
  std::vector<char> fileData = fileLoader.loadData(file);

  //  Парсим загруженные данные
  tinygltf::Model gltfModel;
  _parseGLTF(fileData, gltfModel);

  //  Подготовка к разбору gltf модели
  std::unique_ptr<CommandProducerGraphic> producer =
                                  _uploadingQueue.startCommands("GLTF uploading");
  _producer = producer.get();
  _gltfModel = &gltfModel;
  _currentTansform = glfwToMTTransform;

  //  Отправляем команды на загрузку текстур, которые используются в сцене
  _textures.resize(gltfModel.textures.size());
  for(int textureIndex = 0;
      textureIndex < gltfModel.textures.size();
      textureIndex++)
  {
    _createTexture(textureIndex);
  }

  //  Заранее обходим все материалы и готовим инфу по ним
  _materials.resize(gltfModel.materials.size());
  for(int materialIndex = 0;
      materialIndex < gltfModel.materials.size();
      materialIndex++)
  {
    _createMaterialInfo(materialIndex);
  }

  //  Создаем ассеты мешей
  _meshAssets.resize(gltfModel.meshes.size());
  for(int meshIndex = 0; meshIndex < gltfModel.meshes.size(); meshIndex++)
  {
    _createMeshAssets(meshIndex);
  }

  //  Обходим ноды и создаем дравэйблы
  for(const tinygltf::Scene& scene : gltfModel.scenes)
  {
    for(int nodeIndex : scene.nodes) _processNode(nodeIndex);
  }

  _uploadingQueue.submitCommands(std::move(producer));
}

void GLTFImporter::_parseGLTF(const std::vector<char>& fileData,
                              tinygltf::Model& model)
{
  tinygltf::TinyGLTF gltfLoader;
  _prepareLoader(gltfLoader);

  std::string error;
  std::string warning;
  bool success = gltfLoader.LoadASCIIFromString(
                                          &model,
                                          &error,
                                          &warning,
                                          (const char*)fileData.data(),
                                          (unsigned int)fileData.size(),
                                          pathToUtf8(_baseDir));
  if(!error.empty()) throw std::runtime_error(_filename + " : " + error);
  if(!success) throw std::runtime_error(_filename + " : unable to parse file");
  if(!warning.empty()) Log::warning() << _filename << " : " << warning;
}

void GLTFImporter::_prepareLoader(tinygltf::TinyGLTF& loader) const
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

void GLTFImporter::_createTexture(int textureIndex)
{
  const tinygltf::Texture& gltfTexture = _gltfModel->textures[textureIndex];
  if(gltfTexture.source < 0)
  {
    Log::warning() << _filename << ": texture " << textureIndex << " doesn't have source";
    return;
  }

  tinygltf::Image image = _gltfModel->images[gltfTexture.source];

  if(image.uri.empty())
  {
    Log::warning() << _filename << ": image " << gltfTexture.source << " has empty uri";
    return;
  }
  fs::path imagePath = _baseDir / utf8ToPath(image.uri);

  _textures[textureIndex] = _textureManager.scheduleLoading(imagePath,
                                                            _uploadingQueue,
                                                            true);
}

ConstRef<DataBuffer> GLTFImporter::_uploadData( const void* data,
                                                size_t dataSize,
                                                CommandProducerGraphic& producer,
                                                const std::string& debugName)
{
  Device& device = producer.queue().device();
  ConstRef<DataBuffer> gpuBuffer(new DataBuffer(device,
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

void GLTFImporter::_createMaterialInfo(int materialIndex)
{
  const tinygltf::Material& gltfMaterial = _gltfModel->materials[materialIndex];

  // Режим смешивания по альфе
  GLTFMaterial newMaterial{};
  if(gltfMaterial.alphaMode == "OPAQUE")
  {
    newMaterial.alphaMode = GLTFMaterial::OPAQUE_ALPHA_MODE;
  }
  else if(gltfMaterial.alphaMode == "MASK")
  {
    newMaterial.alphaMode = GLTFMaterial::MASK_ALPHA_MODE;
  }
  else if(gltfMaterial.alphaMode == "BLEND")
  {
    newMaterial.alphaMode = GLTFMaterial::BLEND_ALPHA_MODE;
  }
  else
  {
    newMaterial.alphaMode = GLTFMaterial::OPAQUE_ALPHA_MODE;
    Log::warning() << _filename << " : material: " << gltfMaterial.name << ": unknown alphaMode: " << gltfMaterial.alphaMode;
  }
  newMaterial.alphaCutoff = (float)gltfMaterial.alphaCutoff;

  // Базовые параметры
  newMaterial.doubleSided = gltfMaterial.doubleSided;
  newMaterial.baseColor = glm::vec4(
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
                          gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
  newMaterial.emission = glm::vec3( gltfMaterial.emissiveFactor[0],
                                    gltfMaterial.emissiveFactor[1],
                                    gltfMaterial.emissiveFactor[2]);
  newMaterial.metallic =
                        (float)gltfMaterial.pbrMetallicRoughness.metallicFactor;
  newMaterial.roughness =
                      (float)gltfMaterial.pbrMetallicRoughness.roughnessFactor;
  newMaterial.normalTextureScale = (float)gltfMaterial.normalTexture.scale;
  newMaterial.occlusionTextureStrength =
                                  (float)gltfMaterial.occlusionTexture.strength;

  // Заливаем данные материала на ГПУ
  newMaterial.materialData = _createGPUMaterialInfo(
                                              newMaterial,
                                              _filename+":"+ gltfMaterial.name);

  // Грузим текстуры
  newMaterial.baseColorTexture = _getTexture(
                              gltfMaterial.pbrMetallicRoughness.baseColorTexture,
                              gltfMaterial.name.c_str(),
                              "baseColorTexture");
  newMaterial.metallicRoughnessTexture = _getTexture(
                                            gltfMaterial.pbrMetallicRoughness.
                                                          metallicRoughnessTexture,
                                            gltfMaterial.name.c_str(),
                                            "metallicRoughnessTexture");
  newMaterial.normalTexture = _getTexture(
                      (const tinygltf::TextureInfo&)gltfMaterial.normalTexture,
                      gltfMaterial.name.c_str(),
                      "normalTexture");
  newMaterial.occlusionTexture = _getTexture(
                    (const tinygltf::TextureInfo&)gltfMaterial.occlusionTexture,
                    gltfMaterial.name.c_str(),
                    "occlusionTexture");
  newMaterial.emissiveTexture = _getTexture(gltfMaterial.emissiveTexture,
                                            gltfMaterial.name.c_str(),
                                            "emissiveTexture");
  _materials[materialIndex] = newMaterial;
}

ConstRef<TechniqueResource> GLTFImporter::_getTexture(
                                              const tinygltf::TextureInfo& info,
                                              const char* materialName,
                                              const char* textureName) const
{
  if(info.index < 0) return ConstRef<TechniqueResource>();
  if(info.texCoord != 0) Log::warning() << _filename << " material: " << materialName << " texture:" << textureName << " texture uses custom texture coordinates";
  return _textures[info.index];
}

ConstRef<DataBuffer> GLTFImporter::_createGPUMaterialInfo(
                                            const GLTFMaterial& material,
                                            const std::string& bufferName) const
{
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
  return _uploadData(&gpuData, sizeof(gpuData), *_producer, bufferName);
}

ConstRef<DataBuffer> GLTFImporter::_createAccessorBuffer(
                                            const tinygltf::Accessor& accessor,
                                            const std::string& debugName) const
{
  int32_t componentSize =
                      tinygltf::GetComponentSizeInBytes(accessor.componentType);
  int32_t componentsNum = tinygltf::GetNumComponentsInType(accessor.type);
  int32_t partSize = componentSize * componentsNum;
  size_t dataSize = partSize * accessor.count;
  size_t dataStartOffset = accessor.byteOffset;

  if(accessor.bufferView < 0) throw std::runtime_error(_filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                                  _gltfModel->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(_filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = _gltfModel->buffers[bufferview.buffer];

  if(bufferview.byteStride == partSize || bufferview.byteStride == 0)
  {
    //  В буфере нет разрывов между данными, можем копировать одним куском
    return _uploadData( &buffer.data[dataStartOffset],
                        dataSize,
                        *_producer,
                        debugName);
  }
  else
  {
    //  В буфере есть разрывы, сначала собираем непрерывный буфер, потом
    //  копируем его на GPU
    std::vector<std::byte> accessorData(dataSize);
    std::byte* dstCursor = accessorData.data();
    const unsigned char* srcCursor = &buffer.data[dataStartOffset];
    for(size_t i = 0 ; i < accessor.count; i++)
    {
      memcpy(dstCursor, srcCursor, partSize);
      dstCursor += partSize;
      srcCursor += bufferview.byteStride;
    }
    return _uploadData( accessorData.data(),
                        dataSize,
                        *_producer,
                        debugName);
  }
}

ConstRef<DataBuffer> GLTFImporter::_createIndexBuffer(
                                            const tinygltf::Accessor& accessor,
                                            const std::string& debugName) const
{
  //  Всегда используем 32-х разрядные индексы
  size_t indexSize = 4;

  int32_t srcIndexSize =
                      tinygltf::GetComponentSizeInBytes(accessor.componentType);
  if(srcIndexSize > indexSize) throw std::runtime_error(debugName + ": too big index type");
  size_t dataStartOffset = accessor.byteOffset;

  if(accessor.bufferView < 0) throw std::runtime_error(_filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                                  _gltfModel->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(_filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = _gltfModel->buffers[bufferview.buffer];

  //Создаем промежуточный CPU буфер, чтобы скопировать туда данные идексов
  size_t bufferSize = indexSize * accessor.count;
  std::vector<std::byte> bufferData;
  bufferData.resize(bufferSize, std::byte(0));
  
  // Копируем индексы из исходного буфера
  std::byte* dstCursor = bufferData.data();
  const unsigned char* srcCursor = &buffer.data[dataStartOffset];
  size_t srcStride = bufferview.byteStride == 0 ? srcIndexSize :
                                                  bufferview.byteStride;
  for(size_t i = 0 ; i < accessor.count; i++)
  {
    memcpy(dstCursor, srcCursor, srcIndexSize);
    dstCursor += indexSize;
    srcCursor += srcStride;
  }

  //  Загружаем на ГПУ
  return _uploadData( bufferData.data(),
                      bufferData.size(),
                      *_producer,
                      debugName);
}

void GLTFImporter::_createMeshAssets(int meshIndex)
{
  MeshAssets newSet;

  const tinygltf::Mesh& gltfMesh = _gltfModel->meshes[meshIndex];
  std::string meshName = _filename + ":" + gltfMesh.name;

  //  Для каждого из примитивов создвем свой ассет, так как они могут иметь
  //  разные материалы
  for(const tinygltf::Primitive& primitive : gltfMesh.primitives)
  {
    if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
    {
      Log::warning() << _filename << " : unsupported mesh mode: " << primitive.mode;
      continue;
    }

    Ref<MeshAsset> asset(new MeshAsset(meshName.c_str()));
    VerticesInfo verticesInfo;

    //  Прикрепляем вертекс-буфферы (атрибуты)
    for(std::map<std::string, int>::const_iterator iAttribute =
                                                  primitive.attributes.begin();
        iAttribute != primitive.attributes.end();
        iAttribute++)
    {
      const std::string& attributeName = iAttribute->first;
      const tinygltf::Accessor& accessor =
                                      _gltfModel->accessors[iAttribute->second];
      _processVertexAttribute(attributeName,
                              accessor,
                              *asset,
                              verticesInfo,
                              meshName);
    }

    //  Прикрепляем индексный буфер, если он есть
    if(primitive.indices >= 0)
    {
      const tinygltf::Accessor& accessor =
                                      _gltfModel->accessors[primitive.indices];
      ConstRef<DataBuffer> indicesBuffer = _createIndexBuffer(
                                                        accessor,
                                                        meshName + ":INDICES");
      asset->setCommonBuffer("INDICES", *indicesBuffer);
      verticesInfo.vertexCount = (uint32_t)accessor.count;
      verticesInfo.indicesFound = true;
    }
    
    asset->setVertexCount(verticesInfo.vertexCount);

    if(primitive.material < 0)
    {
      Log::warning() << meshName << " : the primitive doesn't have any material";
      continue;
    }

    //  Подключаем техники
    if(!_attachTechniques(*asset,
                          verticesInfo,
                          _materials[primitive.material],
                          meshName))
    {
      continue;
    }

    newSet.push_back(asset);
  }

  _meshAssets[meshIndex] = std::move(newSet);
}

bool GLTFImporter::_attachTechniques( MeshAsset& targetAsset,
                                      const VerticesInfo& verticesInfo,
                                      const GLTFMaterial& material,
                                      const std::string& meshName)
{
  if(!verticesInfo.positionFound) throw std::runtime_error(meshName + ": POSITION buffer is not found");
  if(!verticesInfo.normalFound) throw std::runtime_error(meshName + ": NORMAL buffer is not found");

  if(material.alphaMode != GLTFMaterial::OPAQUE_ALPHA_MODE)
  {
    Log::warning() << meshName << "Unsupported alpha mode";
    return false;
  }

  if( material.baseColorTexture != nullptr ||
      material.metallicRoughnessTexture != nullptr ||
      material.normalTexture != nullptr ||
      material.occlusionTexture != nullptr ||
      material.emissiveTexture != nullptr)
  {
    if(!verticesInfo.texcoord0Found) throw std::runtime_error(meshName + ": there are some textures in material but TEXCOORD_0 attribute is not found");
  }

  if(material.normalTexture != nullptr && !verticesInfo.tangentFound)
  {
    Log::warning() << meshName << ": there is normal textures in material but TANGENT attribute is not found";
  }

  targetAsset.setCommonBuffer("materialBuffer", *material.materialData);

  if(verticesInfo.indicesFound)
  {
    targetAsset.setCommonSelection("INDICES_ENABLED", "1");
  }
  else targetAsset.setCommonSelection("INDICES_ENABLED", "0");

  if(verticesInfo.texcoord0Found)
  {
    targetAsset.setCommonSelection("TEXCOORD_COUNT", "1");
  }
  else targetAsset.setCommonSelection("TEXCOORD_COUNT", "0");

  if(material.baseColorTexture != nullptr)
  {
    targetAsset.setCommonSelection("BASECOLORTEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("baseColorTexture",
                                  *material.baseColorTexture);
  }
  else targetAsset.setCommonSelection("BASECOLORTEXTURE_ENABLED", "0");

  if(material.metallicRoughnessTexture != nullptr)
  {
    targetAsset.setCommonSelection("METALLICROUGHNESSTEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("metallicRougghnessTexture",
                                  *material.metallicRoughnessTexture);
  }
  else targetAsset.setCommonSelection("METALLICROUGHNESSTEXTURE_ENABLED", "0");

  if(material.normalTexture != nullptr && verticesInfo.tangentFound)
  {
    targetAsset.setCommonSelection("NORMALTEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("normalTexture", *material.normalTexture);
  }
  else targetAsset.setCommonSelection("NORMALTEXTURE_ENABLED", "0");

  if(material.occlusionTexture)
  {
    targetAsset.setCommonSelection("OCCLUSIONTEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("occlusionTexture",
                                  *material.occlusionTexture);
  }
  else targetAsset.setCommonSelection("OCCLUSIONTEXTURE_ENABLED", "0");

  if(material.emissiveTexture)
  {
    targetAsset.setCommonSelection("EMISSIVETEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("emissiveTexture",
                                  *material.emissiveTexture);
  }
  else targetAsset.setCommonSelection("EMISSIVETEXTURE_ENABLED", "0");

  std::unique_ptr<Technique> technique =
                        _techniqueManager.scheduleLoading("gltf/gltfOpaque.tch",
                                                          _device);
  targetAsset.addTechnique(std::move(technique));

  return true;
}

void GLTFImporter::_processVertexAttribute( const std::string& attributeName,
                                            const tinygltf::Accessor& accessor,
                                            MeshAsset& targetAsset,
                                            VerticesInfo& verticesInfo,
                                            const std::string& meshName)
{
  ConstRef<DataBuffer> componentData = _createAccessorBuffer(
                                            accessor,
                                            meshName + ":" + attributeName);
  targetAsset.setCommonBuffer(attributeName.c_str(), *componentData);
  
  if(attributeName == "POSITION")
  {
    //  По этому аттрибуту определяем AABB и сколько вообще у нас есть
    //  вертек сов на отрисовку (если не обнаружим индексный буфер)
    verticesInfo.vertexCount = (uint32_t)accessor.count;
    verticesInfo.positionFound = true;
    if(accessor.minValues.size() == 3 || accessor.maxValues.size() == 3)
    {
      targetAsset.setBound(AABB((float)accessor.minValues[0],
                                (float)accessor.minValues[1],
                                (float)accessor.minValues[2],
                                (float)accessor.maxValues[0],
                                (float)accessor.maxValues[1],
                                (float)accessor.maxValues[2]));
    }
    else Log::warning() << meshName << ":" << attributeName << "doesen't have correct minimal and maximal values";
  }
  else if(attributeName == "NORMAL") verticesInfo.normalFound = true;
  else if(attributeName == "TEXCOORD_0") verticesInfo.texcoord0Found = true;
  else if(attributeName == "TANGENT") verticesInfo.tangentFound = true;
}

void GLTFImporter::_processNode(int nodeIndex)
{
  const tinygltf::Node& node = _gltfModel->nodes[nodeIndex];
  glm::mat4 oldTransform = _currentTansform;
  _applyNodeTransform(node);

  if(node.mesh != -1) _processMesh(node.mesh);

  for(int childIndex : node.children) _processNode(childIndex);

  _currentTansform = oldTransform;
}

void GLTFImporter::_applyNodeTransform(const tinygltf::Node& node)
{
  if(!node.matrix.empty())
  {
    glm::mat4 localTransform(
              node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
              node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
              node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
              node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    _currentTansform = _currentTansform * localTransform;
  }
  else
  {
    if(!node.translation.empty())
    {
      _currentTansform *= glm::translate( glm::mat4(1),
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

      _currentTansform *= glm::toMat4(rotation);
    }
    if(!node.scale.empty())
    {
      _currentTansform *= glm::scale( glm::mat4(1),
                                      glm::vec3(node.scale[0],
                                                node.scale[1],
                                                node.scale[2]));
    }
  }
}

void GLTFImporter::_processMesh(int gltfMeshIndex)
{
  MeshAssets& assets = _meshAssets[gltfMeshIndex];
  for(ConstRef<MeshAsset> asset : assets)
  {
    std::unique_ptr<MeshDrawable> drawable(new MeshDrawable(*asset));
    drawable->setPositionMatrix(_currentTansform);
    _drawables.push_back(std::move(drawable));
  }
}
