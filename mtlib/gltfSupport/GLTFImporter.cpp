#include <cstring>
#include <stdexcept>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <gltfSupport/GLTFImporter.h>
#include <resourceManagement/TechniqueManager.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <vkr/queue/CommandQueueGraphic.h>

namespace fs = std::filesystem;

using namespace mt;

GLTFImporter::GLTFImporter( CommandQueueGraphic& uploadingQueue,
                            TextureManager& textureManager,
                            TechniqueManager& techniqueManager) :
  _uploadingQueue(uploadingQueue),
  _device(uploadingQueue.device()),
  _producer(nullptr),
  _textureManager(textureManager),
  _techniqueManager(techniqueManager),
  _gltfModel(nullptr),
  _currentTansform(1)
{
}

void GLTFImporter::_clear()
{
  _producer = nullptr;
  _drawables.clear();
  _filename.clear();
  _gltfModel = nullptr;
  _meshAssets.clear();
  _currentTansform = glm::mat4(1);
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
  _filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& fileLoader = ContentLoader::getLoader();
  std::vector<char> fileData = fileLoader.loadData(file);

  //  Парсим загруженные данные
  tinygltf::Model gltfModel;
  std::string baseDir = pathToUtf8(file.parent_path());
  _parseGLTF(fileData, gltfModel, baseDir.c_str());

  //  Подготовка к разбору gltf модели
  std::unique_ptr<CommandProducerGraphic> producer =
                                  _uploadingQueue.startCommands("GLTF uploading");
  _producer = producer.get();
  _gltfModel = &gltfModel;
  _currentTansform = glm::mat4(1);

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
                              tinygltf::Model& model,
                              const char* baseDir)
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
                                          baseDir);
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

  if(bufferview.byteStride == partSize)
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

    //  Подключаем техники
    _attachTechniques(*asset, verticesInfo, meshName);

    newSet.push_back(asset);
  }

  _meshAssets[meshIndex] = std::move(newSet);
}

void GLTFImporter::_attachTechniques( MeshAsset& targetAsset,
                                      VerticesInfo& verticesInfo,
                                      const std::string& meshName)
{
  if(!verticesInfo.positionFound) throw std::runtime_error(meshName + ": POSITION buffer is not found");
  if(!verticesInfo.normalFound) throw std::runtime_error(meshName + ": NORMAL buffer is not found");

  std::unique_ptr<Technique> technique =
                        _techniqueManager.scheduleLoading("gltf/gltfOpaque.tch",
                                                          _device);
  targetAsset.addTechnique(std::move(technique));
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
  else if(attributeName == "NORMAL")
  {
    verticesInfo.normalFound = true;
  }
}

void GLTFImporter::_processNode(int nodeIndex)
{
  const tinygltf::Node& node = _gltfModel->nodes[nodeIndex];
  glm::mat4 oldTransform = _currentTansform;
  if(!node.matrix.empty())
  {
    glm::mat4 localTransform(
              node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
              node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
              node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
              node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    _currentTansform = _currentTansform * localTransform;
  }

  if(node.mesh != -1) _processMesh(node.mesh);

  for(int childIndex : node.children) _processNode(childIndex);

  _currentTansform = oldTransform;
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
