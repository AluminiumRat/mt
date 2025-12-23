#pragma once

namespace mt
{
  class BaseWindow;
  class TextureManager;
  class BufferResourceManager;
  class CommandQueueTransfer;
}

struct TechniquePropsWidgetCommon
{
  mt::TextureManager* textureManager;
  mt::BufferResourceManager* bufferManager;
  mt::CommandQueueTransfer* resourceOwnerQueue;
};