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
  const mt::BaseWindow* ownerWindow;
  mt::TextureManager* textureManager;
  mt::BufferResourceManager* bufferManager;
  mt::CommandQueueTransfer* resourceOwnerQueue;
};