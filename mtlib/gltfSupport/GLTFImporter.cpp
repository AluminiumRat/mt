#include <stdexcept>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <gltfSupport/GLTFImporter.h>
#include <resourceManagement/TechniqueManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/Log.h>
#include <vkr/queue/CommandQueueGraphic.h>

namespace fs = std::filesystem;

using namespace mt;

GLTFImporter::GLTFImporter( CommandQueueGraphic& uploadingQueue,
                            TextureManager& textureManager,
                            TechniqueManager& techniqueManager,
                            LoadingPolicy resourcesLoadingPolicy,
                            bool createBLAS) :
  BaseGLTFImporter(uploadingQueue, &textureManager, resourcesLoadingPolicy),
  _producer(nullptr),
  _techniqueManager(techniqueManager),
  _resourcesLoadingPolicy(resourcesLoadingPolicy),
  _createBLAS(createBLAS)
{
  if(resourcesLoadingPolicy == LOAD_ASYNC)
  {
    _whiteTexture = textureManager.scheduleLoading( "util/white.dds",
                                                    uploadingQueue,
                                                    true);
  }
  else
  {
    _whiteTexture = textureManager.loadImmediately( "util/white.dds",
                                                    uploadingQueue,
                                                    true);
  }
}

GLTFImporter::~GLTFImporter() noexcept = default;

void GLTFImporter::_clear() noexcept
{
  _model = nullptr;
  _producer = nullptr;
  _drawables.clear();
  _meshAssets.clear();
  _blasAssets.clear();
}

GLTFImporter::Results GLTFImporter::importGLTF(
                                              const std::filesystem::path& file)
{
  _clear();

  _model = &parseFile(file);
  _meshAssets.resize(_model->meshes.size());
  _blasAssets.resize(_model->meshes.size());

  std::unique_ptr<CommandProducerGraphic> producer =
                              uploadingQueue().startCommands("GLTF uploading");
  _producer = producer.get();

  for(const tinygltf::Scene& scene : _model->scenes)
  {
    for(int nodeIndex : scene.nodes) _processNode(nodeIndex, glfwToMTTransform);
  }

  uploadingQueue().submitCommands(std::move(producer));

  Results results;
  results.drawables = std::move(_drawables);
  results.blases = std::move(_blases);
  return results;
}

void GLTFImporter::_processNode(int nodeIndex, const glm::mat4& parentTransform)
{
  const tinygltf::Node& node = _model->nodes[nodeIndex];
  glm::mat4 nodeTransform = parentTransform * getTransform(node);

  if(node.mesh != -1) _processMesh(node.mesh, nodeTransform);

  for(int childIndex : node.children) _processNode(childIndex, nodeTransform);
}

void GLTFImporter::_processMesh(int gltfMeshIndex, const glm::mat4& tansform)
{
  const MeshAssets& assets = _getMeshAsset(gltfMeshIndex);
  for(ConstRef<MeshAsset> asset : assets)
  {
    std::unique_ptr<MeshDrawable> drawable(new MeshDrawable(*asset));
    drawable->setPositionMatrix(tansform);
    _drawables.push_back(std::move(drawable));
  }

  if(_createBLAS)
  {
    const BLAS* blas = _getBLAS(gltfMeshIndex);
    if(blas != nullptr)
    {
      _blases.push_back(std::unique_ptr<BLASInstance>(
                                    new BLASInstance{ .blas = ConstRef(blas),
                                                      .transform = tansform}));
    }
  }
}

const GLTFImporter::MeshAssets& GLTFImporter::_getMeshAsset(int meshIndex)
{
  if(!_meshAssets[meshIndex].empty()) return _meshAssets[meshIndex];
  _meshAssets[meshIndex] = _createMeshAssets(meshIndex);
  return _meshAssets[meshIndex];
}

GLTFImporter::MeshAssets GLTFImporter::_createMeshAssets(int meshIndex)
{
  MeshAssets newSet;

  const tinygltf::Mesh& gltfMesh = _model->meshes[meshIndex];
  std::string meshName = filename() + ":" + gltfMesh.name;

  //  Для каждого из примитивов создвем свой ассет, так как они могут иметь
  //  разные материалы
  for(const tinygltf::Primitive& primitive : gltfMesh.primitives)
  {
    if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
    {
      Log::warning() << filename() << " : unsupported mesh mode: " << primitive.mode;
      continue;
    }
    if(primitive.material == -1)
    {
      Log::warning() << meshName << " : the primitive doesn't have any material";
      continue;
    }

    VerticesData cpuVertices = getVerticesData(primitive, meshName);
    if(cpuVertices.vertexCount == 0) continue;
    GPUVerticesData gpuVertices = uploadVertices( cpuVertices,
                                                  *_producer,
                                                  meshName);

    const GLTFMaterial& cpuMaterial = getMaterial(primitive.material);
    const DataBuffer& gpuMaterial = getGPUMaterial( primitive.material,
                                                    *_producer);

    Ref<MeshAsset> asset(new MeshAsset(meshName.c_str()));
    if(!_adjustMeshAsset( *asset,
                          gpuVertices,
                          cpuMaterial,
                          gpuMaterial,
                          meshName))
    {
      continue;
    }

    newSet.push_back(asset);
  }

  return newSet;
}

bool GLTFImporter::_adjustMeshAsset(MeshAsset& targetAsset,
                                    const GPUVerticesData& vertices,
                                    const GLTFMaterial& material,
                                    const DataBuffer& gpuMaterial,
                                    const std::string& meshName)
{
  if(vertices.positions == nullptr) throw std::runtime_error(meshName + ": POSITION buffer is not found");
  if(vertices.normals == nullptr) throw std::runtime_error(meshName + ": NORMAL buffer is not found");

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
    if(vertices.texCoord0 == nullptr) throw std::runtime_error(meshName + ": there are some textures in material but TEXCOORD_0 attribute is not found");
  }

  if(material.normalTexture != nullptr && vertices.tangents == nullptr)
  {
    Log::warning() << meshName << ": there is normal textures in material but TANGENT attribute is not found";
  }

  targetAsset.setBound(vertices.bound);

  targetAsset.setCommonBuffer("materialBuffer", gpuMaterial);
  targetAsset.setVertexCount(vertices.vertexCount);
  targetAsset.setCommonBuffer("POSITION", *vertices.positions);
  targetAsset.setCommonBuffer("NORMAL", *vertices.normals);

  if(vertices.indices != nullptr)
  {
    targetAsset.setCommonBuffer("INDICES", *vertices.indices);
    targetAsset.setCommonSelection("INDICES_ENABLED", "1");
  }
  else targetAsset.setCommonSelection("INDICES_ENABLED", "0");

  if(vertices.tangents != nullptr)
  {
    targetAsset.setCommonBuffer("TANGENT", *vertices.tangents);
  }

  //  Шейдеры поддерживают 0,1 или 4 текстурных координат. Если по факту
  //  текстурных координат 2 или 3, то используем texCoord0 вместо недостающих
  if(vertices.texCoord0 == nullptr)
  {
    targetAsset.setCommonSelection("TEXCOORD_COUNT", "0");
  }
  else
  {
    targetAsset.setCommonBuffer("TEXCOORD_0", *vertices.texCoord0);
    if(vertices.texCoord1 == nullptr)
    {
      targetAsset.setCommonSelection("TEXCOORD_COUNT", "1");
    }
    else
    {
      targetAsset.setCommonBuffer("TEXCOORD_1", *vertices.texCoord1);
      if(vertices.texCoord2 != nullptr)
      {
        targetAsset.setCommonBuffer("TEXCOORD_2", *vertices.texCoord2);
      }
      else
      {
        targetAsset.setCommonBuffer("TEXCOORD_2", *vertices.texCoord0);
      }
      if(vertices.texCoord3 != nullptr)
      {
        targetAsset.setCommonBuffer("TEXCOORD_3", *vertices.texCoord3);
      }
      else
      {
        targetAsset.setCommonBuffer("TEXCOORD_3", *vertices.texCoord0);
      }
      targetAsset.setCommonSelection("TEXCOORD_COUNT", "4");
    }
  }

  if(material.baseColorTexture != nullptr)
  {
    targetAsset.setCommonResource("baseColorTexture",
                                  *material.baseColorTexture);
  }
  else
  {
    targetAsset.setCommonResource("baseColorTexture", *_whiteTexture);
  }

  if(material.metallicRoughnessTexture != nullptr)
  {
    targetAsset.setCommonResource("metallicRougghnessTexture",
                                  *material.metallicRoughnessTexture);
  }
  else
  {
    targetAsset.setCommonResource("metallicRougghnessTexture", *_whiteTexture);
  }

  if(material.normalTexture != nullptr)
  {
    targetAsset.setCommonResource("normalTexture", *material.normalTexture);
    if(vertices.tangents != nullptr)
    {
      targetAsset.setCommonSelection( "NORMALTEXTURE_MODE",
                                      "NORMALTEXTURE_VERTEX_TANGENT");
    }
    else
    {
      targetAsset.setCommonSelection( "NORMALTEXTURE_MODE",
                                      "NORMAL_TEXTURE_FRAGMENT_TANGENT");
    }
  }
  else targetAsset.setCommonSelection("NORMALTEXTURE_MODE",
                                      "NORMALTEXTURE_OFF");

  if(material.occlusionTexture)
  {
    targetAsset.setCommonResource("occlusionTexture",
                                  *material.occlusionTexture);
  }
  else
  {
    targetAsset.setCommonResource("occlusionTexture", *_whiteTexture);
  }

  if(material.emissiveTexture)
  {
    targetAsset.setCommonSelection("EMISSIVETEXTURE_ENABLED", "1");
    targetAsset.setCommonResource("emissiveTexture",
                                  *material.emissiveTexture);
  }
  else targetAsset.setCommonSelection("EMISSIVETEXTURE_ENABLED", "0");

  if(_resourcesLoadingPolicy == LOAD_ASYNC)
  {
    targetAsset.addTechnique(
                        _techniqueManager.scheduleLoading("gltf/gltfOpaque.tch",
                                                          device()));
    targetAsset.addTechnique(
                      _techniqueManager.scheduleLoading("gltf/gltfPrepass.tch",
                                                        device()));
  }
  else
  {
    targetAsset.addTechnique(
                        _techniqueManager.loadImmediately("gltf/gltfOpaque.tch",
                                                          device()));
    targetAsset.addTechnique(
                      _techniqueManager.loadImmediately("gltf/gltfPrepass.tch",
                                                        device()));
  }

  return true;
}

const BLAS* GLTFImporter::_getBLAS(int meshIndex)
{
  if(_blasAssets[meshIndex] != nullptr) return _blasAssets[meshIndex].get();

  const tinygltf::Mesh& gltfMesh = _model->meshes[meshIndex];
  std::string meshName = filename() + ":" + gltfMesh.name;

  std::vector<BLASGeometry> buffers;
  for(const tinygltf::Primitive& primitive : gltfMesh.primitives)
  {
    if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
    {
      Log::warning() << filename() << " : unsupported mesh mode: " << primitive.mode;
      continue;
    }
    if(primitive.material == -1)
    {
      Log::warning() << meshName << " : the primitive doesn't have any material";
      continue;
    }

    VerticesData cpuVertices = getVerticesData(primitive, meshName);
    GPUVerticesData gpuVertices = uploadVertices( cpuVertices,
                                                  *_producer,
                                                  meshName);
    if(gpuVertices.positions == nullptr) continue;
    buffers.push_back(BLASGeometry{ .positions = gpuVertices.positions,
                                    .vertexCount = cpuVertices.positions.size(),
                                    .indices = gpuVertices.indices,
                                    .indexCount = cpuVertices.indices.size()});
  }

  if(buffers.empty()) return nullptr;

  Ref<BLAS> newBlas(new BLAS( buffers,
                              meshName.c_str()));
  newBlas->build(*_producer);
  _blasAssets[meshIndex] = newBlas;

  return newBlas.get();
}

ConstRef<DataBuffer> GLTFImporter::createIndexBuffer(
                                                  size_t dataSize,
                                                  const char* bufferName) const
{
  if(_createBLAS)
  {
    return ConstRef(new DataBuffer( device(),
                                    dataSize,
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                    0,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    bufferName));
  }
  else return BaseGLTFImporter::createIndexBuffer(dataSize, bufferName);
}

ConstRef<DataBuffer> GLTFImporter::createVertexBuffer(
                                                  size_t dataSize,
                                                  const char* bufferName) const
{
  if(_createBLAS)
  {
    return ConstRef(new DataBuffer( device(),
                                    dataSize,
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                    0,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    bufferName));
  }
  else return BaseGLTFImporter::createVertexBuffer(dataSize, bufferName);
}