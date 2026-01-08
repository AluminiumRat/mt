#pragma once

namespace mt
{
  class TextureManager;
  class BufferResourceManager;
  class CommandQueueTransfer;

  //  Общие данные для всех пропертей из проперти сета
  struct TechniquePropertySetCommon
  {
    TextureManager* textureManager;
    BufferResourceManager* bufferManager;
    CommandQueueTransfer* resourceOwnerQueue;
  };
}