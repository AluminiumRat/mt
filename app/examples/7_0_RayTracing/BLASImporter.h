#pragma once

#include <vector>

#include <gltfSupport/BaseGLTFImporter.h>
#include <util/Ref.h>

#include <BLASAsset.h>

namespace mt
{
  class BLASImporter : public BaseGLTFImporter
  {
  public:
    BLASImporter(CommandQueueGraphic& uploadingQueue);
    BLASImporter(const BLASImporter&) = delete;
    BLASImporter& operator = (const BLASImporter&) = delete;
    virtual ~BLASImporter() noexcept = default;

    void import(const std::filesystem::path& file);

  private:
    void _clear() noexcept;

    void _processNode(int nodeIndex, const glm::mat4& parentTransform);
    void _processMesh(int gltfMeshIndex, const glm::mat4& tansform);

    const BLASAsset* _getAsset(int meshIndex);

  private:
    const tinygltf::Model* _model;
    CommandProducerGraphic* _producer;

    std::vector<ConstRef<BLASAsset>> _assets;
  };
}