#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <vkr/queue/CommandQueueGraphic.h>

#include <BLASImporter.h>

using namespace mt;

BLASImporter::BLASImporter(CommandQueueGraphic& uploadingQueue) :
  BaseGLTFImporter(uploadingQueue, nullptr, LOAD_IMMEDIATELY)
{
}

void BLASImporter::_clear() noexcept
{
  _model = nullptr;
  _producer = nullptr;
  _assets.clear();
  _instances.clear();
}

BLASInstances BLASImporter::import(const std::filesystem::path& file)
{
  _clear();

  _model = &parseFile(file);
  _assets.resize(_model->meshes.size());

  std::unique_ptr<CommandProducerGraphic> producer =
                              uploadingQueue().startCommands("GLTF uploading");
  _producer = producer.get();

  for(const tinygltf::Scene& scene : _model->scenes)
  {
    for(int nodeIndex : scene.nodes) _processNode(nodeIndex, glfwToMTTransform);
  }

  uploadingQueue().submitCommands(std::move(producer));

  return std::move(_instances);
}

void BLASImporter::_processNode(int nodeIndex, const glm::mat4& parentTransform)
{
  const tinygltf::Node& node = _model->nodes[nodeIndex];
  glm::mat4 nodeTransform = parentTransform * getTransform(node);

  if(node.mesh != -1) _processMesh(node.mesh, nodeTransform);

  for(int childIndex : node.children) _processNode(childIndex, nodeTransform);
}

void BLASImporter::_processMesh(int gltfMeshIndex, const glm::mat4& tansform)
{
  const BLAS* asset = _getAsset(gltfMeshIndex);
  if(asset != nullptr)
  {
    _instances.push_back(BLASInstance{.blas = ConstRef(asset),
                                      .transform = tansform});
  }
}

const BLAS* BLASImporter::_getAsset(int meshIndex)
{
  if(_assets[meshIndex] != nullptr) return _assets[meshIndex].get();

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
  _assets[meshIndex] = newBlas;

  return newBlas.get();
}

ConstRef<DataBuffer> BLASImporter::createIndexBuffer(
                                                  size_t dataSize,
                                                  const char* bufferName) const
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

ConstRef<DataBuffer> BLASImporter::createVertexBuffer(
                                                  size_t dataSize,
                                                  const char* bufferName) const
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