#pragma once

#include <filesystem>

namespace mt
{
  class TextureManager;
  class BufferResourceManager;
  class CommandQueueTransfer;

  struct TechniquePropertyGridCommon
  {
    TextureManager* textureManager;
    BufferResourceManager* bufferManager;
    CommandQueueTransfer* resourceOwnerQueue;
    std::filesystem::path* projectFile;
  };
}