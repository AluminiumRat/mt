#pragma once

// Здесь лежат вспомогательные структуры и типы данных, которые указывают,
// какие очереди команд должны быть созданы и из каких семейств.

#include <array>
#include <optional>
#include <variant>

#include <vkr/queue/QueueFamiliesInfo.h>

namespace mt
{
  class WindowSurface;

  // Типы очередей, которые поддерживаются логическим устройством
  enum QueueType
  {
    GRAPHIC_QUEUE,
    COMPUTE_QUEUE,
    PRESENTATION_QUEUE,
    TRANSFER_QUEUE,
  };
  constexpr int QueueTypeCount = TRANSFER_QUEUE + 1;

  // Тип-метка, говорящий о том, что очередь соответствующего типа создавать
  // не надо. Используется в QueueSource
  struct DoNotCreateQueue {};

  // Информация о том, откуда логическое устройство должно брать какую-либо
  // очередь.
  // Если хранится QueueFamily, то значит, что очередь должна быть заново
  // создана из соответствующего семейства.
  // Если хранится QueueType, значит отдельную очередь создавать не надо,
  // вместо этого следует использовать уже готовую очередь соответствующего
  // типа.
  // Если хранится DoNotCreateQueue, то очередь вообще не надо создавать
  using QueueSource =
                  std::variant<const QueueFamily*, QueueType, DoNotCreateQueue>;

  // Информация о том, какие очереди и откуда должны быть созданы логическим
  // устройством.
  // Можете попробовать создать этот список вручную, но лучше воспользуйтесь
  // makeQueueSources
  using QueueSources = std::array<QueueSource, QueueTypeCount>;

  //  Автогенерация QueueSources для конкретного PhysicalDevice и для
  //    потребностей пользователя
  //  queuesInfo - информация о семействах очередей, поддерживаемых
  //    устройством. Возьмите его у нужного PhysicalDevice
  //  makeGraphic, makeCompute и makePresentation - это
  //    только желание со стороны пользователя иметь отдельные очереди команд.
  //    Если по каким-то причинам не получается создать отдельную очередь,
  //    то какая-то из очередей будет использоваться для нескольких назначений.
  //    Например, если не удастся создать отдельную transfer очередь, то вместо
  //    неё будет использоваться GRAPHIC_QUEUE(или COMPUTE_QUEUE если GRAPHIC_QUE
  //    не создается)
  //  testSurface - тестовая поверхность для проверки, может ли очередь
  //    служить в качестве present очереди. Если указатель не равен nullptr,
  //    то будет выделена(или переиспользована) очередь для презентации.
  //  graphicQueue и computeQueue не могут одновременно быть false (зачем вообще
  //    такое нужно?)
  //  Если для данного queuesInfo не удалось сгенерировать QueueSources,
  //    то будет nullopt
  std::optional<QueueSources> makeQueueSources(
    const QueueFamiliesInfo& familiesInfo,
    bool makeGraphic,
    bool makeCompute,
    bool makeTransfer,
    const WindowSurface* testSurface);
}