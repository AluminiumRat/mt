#pragma once

namespace mt
{
  // Типы очередей, которые поддерживаются логическим устройством
  enum QueueType
  {
    GRAPHIC_QUEUE,
    COMPUTE_QUEUE,
    PRESENTATION_QUEUE,
    TRANSFER_QUEUE,
  };
  constexpr int QueueTypeCount = TRANSFER_QUEUE + 1;
}