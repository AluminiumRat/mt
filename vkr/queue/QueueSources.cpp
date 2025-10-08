#include <exception>
#include <set>

#include <util/Assert.h>
#include <vkr/queue/QueueSources.h>

using namespace mt;

// Слот - это возможнось создания одной очереди из конкретного семейства
// Если семейство поддерживает несколько очередей, то ему соответствет несколько
// слотов.
using Slots = std::vector<const QueueFamily*>;

// Создаем слоты, которые мы можем использовать для создания очередей.
// Фактически размножаем ссылки на каждое семейство столько раз, сколько
// очередей поддерживает семейство
static Slots makeSlots(const QueueFamiliesInfo& familiesInfo)
{
  Slots slots;

  for(const QueueFamily& family : familiesInfo)
  {
    for(uint32_t queueIndex = 0; queueIndex < family.queueCount(); queueIndex++)
    {
      slots.push_back(&family);
    }
  }

  return slots;
}

// Занимаем слот для графической очереди.
static bool findGraphic(Slots& slots, QueueSources& sources)
{
  for(Slots::iterator iSlot = slots.begin();
      iSlot != slots.end();
      iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if(family->isGraphic())
    {
      sources[GRAPHIC_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  Log::info() << "QueueSources: unable to find the graphic queue";
  return false;
}

// Найти подходящий слот для компьют очереди, которая будет выполнять роль
// основной очереди команд (случай, когда графическая очередь не создается)
static bool findPrimaryCompute(Slots& slots, QueueSources& sources)
{
  for(Slots::iterator iSlot = slots.begin();
      iSlot != slots.end();
      iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if(family->isCompute())
    {
      sources[COMPUTE_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  Log::info() << "QueueSources: unable to find the primary compute queue";
  return false;
}

// Определяет, какая из очередей является основной, graphic или compute (случай
// когда graphic очередь не создается вовсе)
static QueueType getPrimaryQueue(const QueueSources& sources) noexcept
{
  // Проверяем, есть ли графическая очередь
  if(std::holds_alternative<const QueueFamily*>(sources[GRAPHIC_QUEUE]))
  {
    return GRAPHIC_QUEUE;
  }

  // Если нет графической очереди, то основной становится компьют очередь
  if (std::holds_alternative<const QueueFamily*>(sources[COMPUTE_QUEUE]))
  {
    return COMPUTE_QUEUE;
  }

  // Если нет ни графической, ни компьют очереди - это ошибка
  MT_ASSERT(false && "getPrimaryQueue: Both the graphic que and the compute queue are nullptr");
}

static bool findPresentationQueue(
  const WindowSurface& testSurface,
  Slots& slots,
  QueueSources& sources)
{
  // Для начала пытаемся найти отдельную очередь
  for(Slots::iterator iSlot = slots.begin();
      iSlot != slots.end();
      iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if(family->isPresentSupported(testSurface))
    {
      sources[PRESENTATION_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  // Если не удалось найти выделенную очередь, то пытаемся использовать
  // основную очередь
  QueueType primaryQueue = getPrimaryQueue(sources);
  const QueueFamily* primaryQueueFamily =
                            std::get<const QueueFamily*>(sources[primaryQueue]);
  if(!primaryQueueFamily->isPresentSupported(testSurface))
  {
    Log::info() << "QueueSources: unable to find any presentation queue";
    return false;
  }
  sources[PRESENTATION_QUEUE] = primaryQueue;
  return true;
}

static bool findTransferQueue(Slots& slots, QueueSources& sources)
{
  // Для начала пытаемся найти специальную трансфер очередь
  for(Slots::iterator iSlot = slots.begin();
      iSlot != slots.end();
      iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if(family->isSeparateTransfer())
    {
      sources[TRANSFER_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  // Если специальную не нашли, то ищем просто отдельную очередь, которая
  // поддерживает трансфер
  for(Slots::iterator iSlot = slots.begin();
      iSlot != slots.end();
      iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if(family->isTransfer())
    {
      sources[TRANSFER_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  //Если не нашли отдельную очередь, то используем основную
  QueueType primaryQueue = getPrimaryQueue(sources);
  sources[TRANSFER_QUEUE] = primaryQueue;
  return true;
}

// Найти дополнительную компьют очередь. Вызывается в случае, если нужны и
// графическая очередь, и компьют
static bool findSecondaryComputeQueue(Slots& slots, QueueSources& sources)
{
  // Для начала пытаемся найти отдельную очередь
  for ( Slots::iterator iSlot = slots.begin();
        iSlot != slots.end();
        iSlot++)
  {
    const QueueFamily* family = *iSlot;
    if (family->isCompute())
    {
      sources[COMPUTE_QUEUE] = family;
      slots.erase(iSlot);
      return true;
    }
  }

  //Если не нашли отдельную очередь, то используем основную
  QueueType primaryQueue = getPrimaryQueue(sources);
  const QueueFamily* primaryQueueFamily =
                            std::get<const QueueFamily*>(sources[primaryQueue]);
  if(!primaryQueueFamily->isCompute())
  {
    Log::info() << "QueueSources: unable to find any compute queue";
    return false;
  }
  sources[COMPUTE_QUEUE] = primaryQueue;
  return true;
}

std::optional<QueueSources> mt::makeQueueSources(
  const QueueFamiliesInfo& familiesInfo,
  bool makeGraphic,
  bool makeCompute,
  bool makeTransfer,
  const WindowSurface* testSurface)
{
  MT_ASSERT((makeGraphic || makeCompute) && "You must create at least one of these queues");

  QueueSources sources{
    DoNotCreateQueue(),
    DoNotCreateQueue(),
    DoNotCreateQueue(),
    DoNotCreateQueue()};

  Slots slots = makeSlots(familiesInfo);

  // Создаем основную очередь. Это может быть как графическая, так и компьют
  if(makeGraphic)
  { 
    if(!findGraphic(slots, sources)) return std::nullopt;
  }
  else
  {
    if(!findPrimaryCompute(slots, sources)) return std::nullopt;
  }

  // Очередб презентации
  if(testSurface != nullptr)
  {
    if(!findPresentationQueue(*testSurface, slots, sources))
    {
      return std::nullopt;
    }
  }

  // Трансфер очередь
  if(makeTransfer)
  {
    if(!findTransferQueue(slots, sources)) return std::nullopt;
  }

  // Выделенная компьют очередь
  if(makeGraphic && makeCompute)
  {
    if(!findSecondaryComputeQueue(slots, sources)) return std::nullopt;
  }

  return sources;
}