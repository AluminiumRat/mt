#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/ImageAccess.h>
#include <vkr/Image.h>

using namespace mt;

//  Добавляет барьер памяти, разделяющий два доступа
void MemoryAccess::createBarrier(
                            const MemoryAccess& nextAccess,
                            const CommandBuffer& commandBuffer) const noexcept
{
  if(nextAccess.writeStagesMask == 0)
  {
    //  В следующем доступе нет записи в память, поэтому нет смысла зищищать
    //  чтение в предыдущем
    commandBuffer.memoryBarrier(writeStagesMask,
                                nextAccess.readStagesMask,
                                writeAccessMask,
                                nextAccess.readAccessMask);
  }
  else
  {
    if(writeStagesMask == 0)
    {
      //  В последующем доступе есть запись, но в предыдущем записи нет.
      //  Защищаем только чтение в предыдущем доступе
      commandBuffer.pipelineBarrier(readStagesMask,
                                    nextAccess.writeStagesMask);
    }
    else
    {
      // И в предыдущем и в последующем доступах есть запись, защищаем
      // по максимуму
      commandBuffer.memoryBarrier(
                        readStagesMask | writeStagesMask,
                        nextAccess.writeStagesMask | nextAccess.readStagesMask,
                        writeAccessMask,
                        nextAccess.readAccessMask);
    }
  }
}

void MemoryAccess::updateBarrierMasks(const MemoryAccess& nextAccess,
                                      VkPipelineStageFlags& srcStages,
                                      VkPipelineStageFlags& dstStages,
                                      VkAccessFlags& srcAccess,
                                      VkAccessFlags& dstAccess) const noexcept
{
  if(nextAccess.writeStagesMask == 0)
  {
    //  В следующем доступе нет записи в память, поэтому нет смысла зищищать
    //  чтение в предыдущем
    srcStages |= writeStagesMask;
    dstStages |= nextAccess.readStagesMask;
    srcAccess |= writeAccessMask;
    dstAccess |= nextAccess.readAccessMask;
  }
  else
  {
    if(writeStagesMask == 0)
    {
      //  В последующем доступе есть запись, но в предыдущем записи нет.
      //  Защищаем только чтение в предыдущем доступе
      srcStages |= readStagesMask;
      dstStages |= nextAccess.writeStagesMask;
    }
    else
    {
      // И в предыдущем и в последующем доступах есть запись данных, изолируем
      // по максимуму
      srcStages |= readStagesMask | writeStagesMask;
      dstStages |= nextAccess.writeStagesMask | nextAccess.readStagesMask;
      srcAccess |= writeAccessMask;
      dstAccess |= nextAccess.readAccessMask;
    }
  }
}

bool ImageAccess::mergeNoBarriers(const ImageAccess& nextAccess) noexcept
{
  //  Слияние доступов по памяти делаем в отдельном аррее, чтобы не испортить
  //  актуальные данные, если слить доступы не удастся
  std::array<MemoryAccess, maxSlices> newMemoryAccesses = memoryAccess;

  //  Обходим все слайсы следующего доступа и ищем слайсы в предыдущем, которые
  //  мы можем переиспользовать
  for ( uint32_t iRequiredSlice = 0;
        iRequiredSlice < nextAccess.slicesCount;
        iRequiredSlice++)
  {
    const ImageSlice& requiredSlice = nextAccess.slices[iRequiredSlice];
    VkImageLayout requiredLayout = nextAccess.layouts[iRequiredSlice];
    const MemoryAccess& requiredAccess =
                                        nextAccess.memoryAccess[iRequiredSlice];

    bool sliceIsMerged = false;
    for ( uint32_t iMySlice = 0; iMySlice < slicesCount; iMySlice++)
    {
      if( layouts[iMySlice] == requiredLayout &&
          slices[iMySlice].contains(requiredSlice) &&
          !newMemoryAccesses[iMySlice].needBarrier(requiredAccess))
      {
        newMemoryAccesses[iMySlice] = newMemoryAccesses[iMySlice].merge(
                                                                requiredAccess);
        sliceIsMerged = true;
        break;
      }
    }

    if(!sliceIsMerged) return false;
  }

  memoryAccess = newMemoryAccesses;
  return true;
}

ImageAccess::TransformHint ImageAccess::mergeWithBarriers(
                                        const ImageAccess& nextAccess) noexcept
{
  //  Для начала обойдем все уже существующие слайсы и сбросим им доступ по
  //    памяти. По умолчанию считаем, что они не используются в следующем
  //    доступе.
  //  Те, кто будет переиспользоваться, получат новые флаги доступа
  for (uint32_t iMySlice = 0; iMySlice < slicesCount; iMySlice++)
  {
    memoryAccess[iMySlice] = MemoryAccess{};
  }

  //  Обходим все слайсы следующего доступа и добавляем в текущий
  for ( uint32_t iRequiredSlice = 0;
        iRequiredSlice < nextAccess.slicesCount;
        iRequiredSlice++)
  {
    const ImageSlice& requiredSlice = nextAccess.slices[iRequiredSlice];
    VkImageLayout requiredLayout = nextAccess.layouts[iRequiredSlice];
    const MemoryAccess& requiredAccess =
                                        nextAccess.memoryAccess[iRequiredSlice];

    //  Ищем слайсы в предыдущем доступе, которые мы можем переиспользовать
    bool sliceIsMerged = false;
    for (uint32_t iMySlice = 0; iMySlice < slicesCount; iMySlice++)
    {
      const ImageSlice& mySlice = slices[iMySlice];
      if(requiredSlice.isIntersected(mySlice))
      {
        if(mySlice != requiredSlice || layouts[iMySlice] != requiredLayout)
        {
          // Обнаружили конфликт слайсов. Будем полностью переделывать лэйауты
          *this = nextAccess;
          return RESET_SLICES;
        }
        //  Мы переиспользуем слайс в следующем доступе, поэтому ставим ему
        //  новые флаги доступа по памяти
        memoryAccess[iMySlice].merge(requiredAccess);
        sliceIsMerged = true;
        break;
      }
    }

    // Если не удалось переиспользовать старые слайсы, вытаемся добавить новый
    if(!sliceIsMerged)
    {
      if(slicesCount == maxSlices)
      {
        *this = nextAccess;
        return RESET_SLICES;
      }
      slices[slicesCount] = requiredSlice;
      layouts[slicesCount] = requiredLayout;
      memoryAccess[slicesCount] = requiredAccess;
      slicesCount++;
    }
  }

  return REUSE_SLICES;
}

void MatchingPoint::makeMatch(const Image& image,
                              const ImageAccess& nextAccess) const noexcept
{
  MT_ASSERT(matchingBuffer != nullptr);

  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

  VkImageMemoryBarrier imageBarriers[2 * ImageAccess::maxSlices];
  uint32_t barriersCount = 0;

  VkPipelineStageFlags srcStages = 0;
  VkPipelineStageFlags dstStages = 0;

  uint32_t firstNewSliceIndex = 0;  //Индекс, с которого начинаются новые слайсы

  if(transformHint == ImageAccess::RESET_SLICES)
  {
    // Сбрасываем старые слайсы
    for(uint32_t i = 0; i < previousAccess.slicesCount; i++)
    {
      const ImageSlice& slice = previousAccess.slices[i];
      VkImageLayout layout = previousAccess.layouts[i];
      const MemoryAccess& memoryAccess = previousAccess.memoryAccess[i];

      srcStages |= memoryAccess.readStagesMask | memoryAccess.writeStagesMask;
      memoryBarrier.srcAccessMask |= memoryAccess.writeAccessMask;

      imageBarriers[barriersCount] = VkImageMemoryBarrier{};
      imageBarriers[barriersCount].sType =
                                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageBarriers[barriersCount].oldLayout = layout;
      imageBarriers[barriersCount].newLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageBarriers[barriersCount].image = image.handle();
      imageBarriers[barriersCount].subresourceRange = slice.makeRange();
      barriersCount++;
    }
  }
  else
  {
    // Переиспользуем старые слайсы, избавляемся от возможных конфликтов по
    // памяти
    for (uint32_t i = 0; i < previousAccess.slicesCount; i++)
    {
      previousAccess.memoryAccess[i].updateBarrierMasks(
                                                  nextAccess.memoryAccess[i],
                                                  srcStages,
                                                  dstStages,
                                                  memoryBarrier.srcAccessMask,
                                                  memoryBarrier.dstAccessMask);
    }
    firstNewSliceIndex = previousAccess.slicesCount;
  }

  // Добавляем новые слайсы
  for(uint32_t i = firstNewSliceIndex; i < nextAccess.slicesCount; i++)
  {
    const ImageSlice& slice = nextAccess.slices[i];
    VkImageLayout layout = nextAccess.layouts[i];
    const MemoryAccess& memoryAccess = nextAccess.memoryAccess[i];

    dstStages |= memoryAccess.readStagesMask | memoryAccess.writeStagesMask;
    memoryBarrier.dstAccessMask |= memoryAccess.readAccessMask;

    imageBarriers[barriersCount] = VkImageMemoryBarrier{};
    imageBarriers[barriersCount].sType =
                                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarriers[barriersCount].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarriers[barriersCount].newLayout = layout;
    imageBarriers[barriersCount].image = image.handle();
    imageBarriers[barriersCount].subresourceRange = slice.makeRange();
    barriersCount++;
  }

  vkCmdPipelineBarrier( matchingBuffer->handle(),
                        srcStages,
                        dstStages,
                        0,
                        1,
                        &memoryBarrier,
                        0,
                        nullptr,
                        barriersCount,
                        imageBarriers);
}
