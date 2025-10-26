#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/ImageAccess.h>
#include <vkr/Image.h>

using namespace mt;

//  Добавляет барьер памяти, разделяющий два доступа
void ResourceAccess::createBarrier(
                            const ResourceAccess& nextAccess,
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

void ApprovingPoint::makeApprove( const Image& image,
                                  const ImageAccess& nextAccess) const noexcept
{
  MT_ASSERT(approvingBuffer != nullptr);

  if(!layoutTranslation.needToDoAnything())
  {
    //  Преобразовывать лэйауты не нужно, ApprovingPoint был создан из-за
    //  конфликтов доступа к памяти.
    previousAccess.memoryAccess.createBarrier(nextAccess.memoryAccess,
                                              *approvingBuffer);
    return;
  }

  //  Надо преобразовывать layout-ы
  VkImageMemoryBarrier barriers[2];
  uint32_t barriersCount = 0;

  if(layoutTranslation.needToCollapseSlice)
  {
    // Возвращаем changedSlice в общий лэйаут
    MT_ASSERT(previousAccess.requiredLayouts.changedSlice.has_value());
    MT_ASSERT(&previousAccess.requiredLayouts.changedSlice->image() == &image);

    barriers[0] = VkImageMemoryBarrier{};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].oldLayout = previousAccess.requiredLayouts.changedSliceLayout;
    barriers[0].newLayout = previousAccess.requiredLayouts.primaryLayout;
    barriers[0].image = image.handle();
    barriers[0].subresourceRange =
                      previousAccess.requiredLayouts.changedSlice->makeRange();
    barriers[0].srcAccessMask = previousAccess.memoryAccess.writeAccessMask;
    barriers[0].dstAccessMask = nextAccess.memoryAccess.readAccessMask;
    barriersCount++;
  }

  if(layoutTranslation.needToCreateSlice)
  {
    // Создаем новый changedSlice, либо меняем лэйаут сразу всего Image-а
    ImageSlice slice = nextAccess.requiredLayouts.changedSlice.has_value() ?
                                      *nextAccess.requiredLayouts.changedSlice :
                                      ImageSlice(image);
    VkImageLayout newLayout =
                    nextAccess.requiredLayouts.changedSlice.has_value() ?
                                nextAccess.requiredLayouts.changedSliceLayout :
                                nextAccess.requiredLayouts.primaryLayout;

    barriers[0] = VkImageMemoryBarrier{};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].oldLayout = previousAccess.requiredLayouts.primaryLayout;
    barriers[0].newLayout = newLayout;
    barriers[0].image = image.handle();
    barriers[0].subresourceRange = slice.makeRange();
    barriers[0].srcAccessMask = previousAccess.memoryAccess.writeAccessMask;
    barriers[0].dstAccessMask = nextAccess.memoryAccess.readAccessMask;
    barriersCount++;
  }

  MT_ASSERT(barriersCount != 0);

  // Защищаем все используемые стадии пайплайна, так как сам барьер может
  // читать и писать в память
  vkCmdPipelineBarrier( approvingBuffer->handle(),
                        previousAccess.memoryAccess.readStagesMask |
                                    previousAccess.memoryAccess.writeStagesMask,
                        nextAccess.memoryAccess.readStagesMask |
                                        nextAccess.memoryAccess.writeStagesMask,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        barriersCount,
                        barriers);
}
