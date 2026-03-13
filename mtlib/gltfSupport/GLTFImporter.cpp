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
                            LoadingPolicy resourcesLoadingPolicy) :
  BaseGLTFImporter(uploadingQueue, &textureManager, resourcesLoadingPolicy),
  _producer(nullptr),
  _techniqueManager(techniqueManager),
  _resourcesLoadingPolicy(resourcesLoadingPolicy)
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
}

GLTFImporter::DrawablesList GLTFImporter::importGLTF(
                                              const std::filesystem::path& file)
{
  _clear();

  _model = &parseFile(file);
  _meshAssets.resize(_model->meshes.size());

  std::unique_ptr<CommandProducerGraphic> producer =
                              uploadingQueue().startCommands("GLTF uploading");
  _producer = producer.get();

  for(const tinygltf::Scene& scene : _model->scenes)
  {
    for(int nodeIndex : scene.nodes) _processNode(nodeIndex, glfwToMTTransform);
  }

  uploadingQueue().submitCommands(std::move(producer));
  return std::move(_drawables);
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
  const MeshAssets& assets = _getAsset(gltfMeshIndex);
  for(ConstRef<MeshAsset> asset : assets)
  {
    std::unique_ptr<MeshDrawable> drawable(new MeshDrawable(*asset));
    drawable->setPositionMatrix(tansform);
    _drawables.push_back(std::move(drawable));
  }
}

const GLTFImporter::MeshAssets& GLTFImporter::_getAsset(int meshIndex)
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
    if(!_adjustAsset( *asset, gpuVertices, cpuMaterial, gpuMaterial, meshName))
    {
      continue;
    }

    newSet.push_back(asset);
  }

  return newSet;
}

bool GLTFImporter::_adjustAsset(MeshAsset& targetAsset,
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