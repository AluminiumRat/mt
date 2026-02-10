#include <cstring>
#include <stdexcept>

#include <glm/glm.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <gltfSupport/gltfSupport.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <resourceManagement/TechniqueManager.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <util/Ref.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/DataBuffer.h>

namespace fs = std::filesystem;

using namespace mt;

//  Набор hld ассетов для одного gltf меша
using AssetSet = std::vector<ConstRef<MeshAsset>>;
//  Набор маш асетов для всей сцены. Индекс в векторе соответствует индексу
//  меша в tinygltf::Model
using AssetLib = std::vector<AssetSet>;

struct BuildSceneContext
{
  std::string filename;
  DrawScene* targetScene;
  std::vector<std::unique_ptr<Drawable>>* drawablesContainer;
  CommandProducerGraphic* producer;
  Device* device;
  TextureManager* textureManager;
  TechniqueManager* techniqueManager;
  const tinygltf::Model* gltfModel;
  AssetLib* assets;
  glm::mat4 transformMatrix;
};

static ConstRef<DataBuffer> uploadData( const void* data,
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

static ConstRef<DataBuffer> createAccessorBuffer(
                                            const tinygltf::Accessor& accessor,
                                            BuildSceneContext& context,
                                            const std::string& debugName)
{
  int32_t componentSize =
                      tinygltf::GetComponentSizeInBytes(accessor.componentType);
  int32_t componentsNum = tinygltf::GetNumComponentsInType(accessor.type);
  int32_t partSize = componentSize * componentsNum;
  size_t dataSize = partSize * accessor.count;
  size_t dataStartOffset = accessor.byteOffset;

  if(accessor.bufferView < 0) throw std::runtime_error(context.filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                            context.gltfModel->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(context.filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = context.gltfModel->buffers[bufferview.buffer];

  if(bufferview.byteStride == partSize)
  {
    //  В буфере нет разрывов между данными, можем копировать одним куском
    return uploadData(&buffer.data[dataStartOffset],
                      dataSize,
                      *context.producer,
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
    return uploadData(accessorData.data(),
                      dataSize,
                      *context.producer,
                      debugName);
  }
}

static ConstRef<DataBuffer> createIndexBuffer(
                                            const tinygltf::Accessor& accessor,
                                            BuildSceneContext& context,
                                            const std::string& debugName)
{
  //  Всегда используем 32-х разрядные индексы
  size_t indexSize = 4;

  int32_t srcIndexSize =
                      tinygltf::GetComponentSizeInBytes(accessor.componentType);
  if(srcIndexSize > indexSize) throw std::runtime_error(debugName + ": too big index type");
  size_t dataStartOffset = accessor.byteOffset;

  if(accessor.bufferView < 0) throw std::runtime_error(context.filename + " : sparse accessor found. Sparse accessor aren't supported.");
  const tinygltf::BufferView& bufferview =
                            context.gltfModel->bufferViews[accessor.bufferView];
  dataStartOffset += bufferview.byteOffset;

  if(bufferview.buffer < 0) throw std::runtime_error(context.filename + " : wrong buffer reference");
  const tinygltf::Buffer& buffer = context.gltfModel->buffers[bufferview.buffer];

  //Создаем промежуточный CPU буфер, чтобы скопировать туда данные ндексов
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
  return uploadData(bufferData.data(),
                    bufferData.size(),
                    *context.producer,
                    debugName);
}

static void createMeshAssets(BuildSceneContext& context, int meshIndex)
{
  AssetSet newSet;

  const tinygltf::Mesh& gltfMesh = context.gltfModel->meshes[meshIndex];
  std::string meshName = context.filename + ":" + gltfMesh.name;

  for(const tinygltf::Primitive& primitive : gltfMesh.primitives)
  {
    if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
    {
      Log::warning() << context.filename << " : unsupported mesh mode: " << primitive.mode;
      continue;
    }

    Ref<MeshAsset> asset(new MeshAsset(meshName.c_str()));

    uint32_t vertexCount = 0;

    //  Прикрепляем вертекс-буфферы (атрибуты)
    for(std::map<std::string, int>::const_iterator iAttribute =
                                                  primitive.attributes.begin();
        iAttribute != primitive.attributes.end();
        iAttribute++)
    {
      const std::string& attributeName = iAttribute->first;
      const tinygltf::Accessor& accessor =
                              context.gltfModel->accessors[iAttribute->second];
      ConstRef<DataBuffer> componentData = createAccessorBuffer(
                                                accessor,
                                                context,
                                                meshName + ":" + attributeName);
      asset->setCommonBuffer(attributeName.c_str(), *componentData);
      if(attributeName == "POSITION")
      {
        vertexCount = (uint32_t)accessor.count;
        if(accessor.minValues.size() == 3 || accessor.maxValues.size() == 3)
        {
          asset->setBound(AABB( (float)accessor.minValues[0],
                                (float)accessor.minValues[1],
                                (float)accessor.minValues[2],
                                (float)accessor.maxValues[0],
                                (float)accessor.maxValues[1],
                                (float)accessor.maxValues[2]));
        }
        else Log::warning() << meshName << ":" << attributeName << "doesen't have correct minimal and maximal values";
      }
    }

    //  Прикрепляем индексный буфер, если есть
    if(primitive.indices >= 0)
    {
      const tinygltf::Accessor& accessor =
                                context.gltfModel->accessors[primitive.indices];
      ConstRef<DataBuffer> indicesBuffer = createIndexBuffer(
                                                        accessor,
                                                        context,
                                                        meshName + ":INDICES");
      asset->setCommonBuffer("INDICES", *indicesBuffer);
      vertexCount = (uint32_t)accessor.count;
    }
    
    asset->setVertexCount(vertexCount);

    //  Подключаем технику
    std::unique_ptr<Technique> technique =
                      context.techniqueManager->scheduleLoading(
                                                          "gltf/gltfOpaque.tch",
                                                          *context.device);
    asset->addTechnique(std::move(technique));

    newSet.push_back(asset);
  }

  (*context.assets)[meshIndex] = std::move(newSet);
}

static void processMesh(BuildSceneContext& context, int gltfMeshIndex)
{
  AssetSet& assets = (*context.assets)[gltfMeshIndex];
  for(ConstRef<MeshAsset> asset : assets)
  {
    std::unique_ptr<MeshDrawable> drawable(new MeshDrawable(*asset));
    drawable->setPositionMatrix(context.transformMatrix);
    context.drawablesContainer->push_back(std::move(drawable));
    context.targetScene->registerDrawable(*context.drawablesContainer->back());
  }
}

static void processNode(BuildSceneContext& context, int nodeIndex)
{
  const tinygltf::Node& node = context.gltfModel->nodes[nodeIndex];
  glm::mat4 oldTransform = context.transformMatrix;
  if(!node.matrix.empty())
  {
    glm::mat4 localTransform(
              node.matrix[0],  node.matrix[1],  node.matrix[2],  node.matrix[3],
              node.matrix[4],  node.matrix[5],  node.matrix[6],  node.matrix[7],
              node.matrix[8],  node.matrix[9],  node.matrix[10], node.matrix[11],
              node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    context.transformMatrix = context.transformMatrix * localTransform;
  }

  if(node.mesh != -1) processMesh(context, node.mesh);

  for(int childIndex : node.children) processNode(context, childIndex);

  context.transformMatrix = oldTransform;
}

void mt::loadGLTFScene(
                    fs::path file,
                    DrawScene& targetScene,
                    std::vector<std::unique_ptr<Drawable>>& drawablesContainer,
                    CommandQueueGraphic& uploadingQueue,
                    TextureManager& textureManager,
                    TechniqueManager& techniqueManager)
{
  std::string filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& fileLoader = ContentLoader::getLoader();
  std::vector<char> fileData = fileLoader.loadData(file);

  //  Парсим загруженные данные
  tinygltf::Model model;
  tinygltf::TinyGLTF gltfLoader;

  tinygltf::FsCallbacks fsCallbacks{};
  fsCallbacks.FileExists =
    [&](const std::string filename, void*)
    {
      return fileLoader.exists(utf8ToPath(filename));
    };

  fsCallbacks.ExpandFilePath =  [](const std::string filename, void*)
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
  fsCallbacks.WriteWholeFile =  []( std::string*,
                                    const std::string&,
                                    const std::vector<unsigned char>&,
                                    void*)
                                {return false;};
  fsCallbacks.GetFileSizeInBytes =  []( size_t* filesize_out,
                                        std::string* err,
                                        const std::string& abs_filename,
                                        void* userdata)
                                    {
                                      //  Этот калбэк нужен только для проверки
                                      //  на максимальный размер, поэтому просто
                                      //  вернем 0
                                      *filesize_out = 0;
                                      return true;
                                    };
  gltfLoader.SetFsCallbacks(fsCallbacks);

  std::string error;
  std::string warning;
  std::string baseDir = pathToUtf8(file.parent_path());
  bool success = gltfLoader.LoadASCIIFromString(
                                          &model,
                                          &error,
                                          &warning,
                                          (const char*)fileData.data(),
                                          (unsigned int)fileData.size(),
                                          baseDir.c_str());

  if(!error.empty()) throw std::runtime_error(filename + " : " + error);
  if(!success) throw std::runtime_error(filename + " : unable to parse file");
  if(!warning.empty()) Log::warning() << filename << " : " << warning;

  std::unique_ptr<CommandProducerGraphic> producer =
                                  uploadingQueue.startCommands("GLTF uploading");

  BuildSceneContext buildContext{};
  buildContext.filename = filename;
  buildContext.targetScene = &targetScene;
  buildContext.drawablesContainer = &drawablesContainer;
  buildContext.producer = producer.get();
  buildContext.device = &uploadingQueue.device();
  buildContext.textureManager = &textureManager;
  buildContext.techniqueManager = &techniqueManager;
  buildContext.gltfModel = &model;
  buildContext.transformMatrix = glm::mat4(1);

  //  Создаем ассеты мешей
  AssetLib meshAssets;
  meshAssets.resize(model.meshes.size());
  buildContext.assets = &meshAssets;
  for(int meshIndex = 0; meshIndex < model.meshes.size(); meshIndex++)
  {
    createMeshAssets(buildContext, meshIndex);
  }

  //  Обходим ноды
  for(const tinygltf::Scene& scene : model.scenes)
  {
    for(int nodeIndex : scene.nodes) processNode(buildContext, nodeIndex);
  }

  uploadingQueue.submitCommands(std::move(producer));
}
