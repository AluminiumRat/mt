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
}

void BLASImporter::import(const std::filesystem::path& file)
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
  const BLASAsset* asset = _getAsset(gltfMeshIndex);
}

const BLASAsset* BLASImporter::_getAsset(int meshIndex)
{
  if(_assets[meshIndex] != nullptr) return _assets[meshIndex].get();

  const tinygltf::Mesh& gltfMesh = _model->meshes[meshIndex];
  std::string meshName = filename() + ":" + gltfMesh.name;

  std::vector<BLASAsset::Geometry> buffers;
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
    buffers.push_back(BLASAsset::Geometry{.positions = gpuVertices.positions,
                                          .indices = gpuVertices.indices});
  }

  if(buffers.empty()) return nullptr;

  ConstRef<BLASAsset> newBlas(new BLASAsset(buffers));
  _assets[meshIndex] = newBlas;

  return newBlas.get();
}
