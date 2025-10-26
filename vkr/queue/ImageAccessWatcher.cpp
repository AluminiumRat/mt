#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/ImageAccessWatcher.h>

using namespace mt;

ImageAccessWatcher::ImageAccessWatcher() noexcept :
  _accessMap(1087),
  _isFinalized(false)
{
}

ImageAccessWatcher::MemoryConflict ImageAccessWatcher::addImageAccess(
                                        const SliceAccess& sliceAccess,
                                        CommandBuffer& approvingBuffer) noexcept
{
  MT_ASSERT(!_isFinalized);

  try
  {
    const Image& image = sliceAccess.slice.image();

    auto insertion = _accessMap.emplace(&image, ImageAccessLayoutState());
    bool isNewRecord = insertion.second;
    ImageAccessLayoutState& imageState = insertion.first->second;

    if(isNewRecord)
    {
      //  Image ещё не использовался. Просто заполняем требования доступа
      imageState.currentAccess.requiredLayouts.primaryLayout =
                                                    sliceAccess.requiredLayout;
      imageState.currentAccess.memoryAccess = sliceAccess.memoryAccess;
      return NO_MEMORY_CONFLICT;
    }

    ImageAccess& currentImageAccess = imageState.currentAccess;

    // Смотрим, что нужно сделать, чтобы получить слайс в нужном лэйауте
    LayoutTranslation layoutTranslation =
                                  getLayoutTranslation(
                                            currentImageAccess.requiredLayouts,
                                            sliceAccess.slice,
                                            sliceAccess.requiredLayout);
    if(!layoutTranslation.translationType.needToDoAnything())
    {
      // Лэйауты менять не надо. Возможно есть конфликт доступа к памяти
      if(!currentImageAccess.memoryAccess.needBarrier(sliceAccess.memoryAccess))
      {
        // Барьер добавлять не надо. Просто добавляем требования по доступу
        currentImageAccess.memoryAccess.merge(sliceAccess.memoryAccess);
        return NO_MEMORY_CONFLICT;
      }
    }

    _addApprovingPoint( image,
                        imageState,
                        approvingBuffer,
                        layoutTranslation,
                        sliceAccess.memoryAccess);
    return NEED_TO_APPROVE;
  }
  catch (std::exception& error)
  {
    Log::error() << "ImageAccessWatcher::addImageAccess: unable to add image access: " << error.what();
    MT_ASSERT(false && "Unable to add image access");
  }
}

void ImageAccessWatcher::_addApprovingPoint(
                                        const Image& image,
                                        ImageAccessLayoutState& imageState,
                                        CommandBuffer& approvingBuffer,
                                        const LayoutTranslation& translation,
                                        const ResourceAccess& newMemoryAccess)
{
  if(imageState.lastApprovingPoint.has_value())
  {
    // Существует не законченная точка апрува. Закроем её, так как мы уже
    // знаем про все доступы в текущей серии(мы её сейчас закроем)
    imageState.lastApprovingPoint->makeApprove(image, imageState.currentAccess);
    imageState.lastApprovingPoint.reset();
  }
  else
  {
    // Это был первая серия доступов, надо сохранить её
    imageState.initialAccess = imageState.currentAccess;
  }

  imageState.lastApprovingPoint = ApprovingPoint{
                              .approvingBuffer = &approvingBuffer,
                              .previousAccess = imageState.currentAccess,
                              .layoutTranslation = translation.translationType};

  // Обновляем текущую серию доступов
  imageState.currentAccess.requiredLayouts = translation.nextState;
  imageState.currentAccess.memoryAccess = newMemoryAccess;
}

const ImageAccessMap& ImageAccessWatcher::finalize() noexcept
{
  // Выставить initailAcces у всех image, у кого не выставлен
  // Закрыть все ApprovingPoint-ы
  try
  {
    for(ImageAccessMap::iterator iImageState = _accessMap.begin();
        iImageState != _accessMap.end();
        iImageState++)
    {
      const Image* image = iImageState->first;
      ImageAccessLayoutState& imageState = iImageState->second;

      if(!imageState.initialAccess.has_value())
      {
        imageState.initialAccess = imageState.currentAccess;
      }

      if(imageState.lastApprovingPoint.has_value())
      {
        imageState.lastApprovingPoint->makeApprove( *image,
                                                    imageState.currentAccess);
        imageState.lastApprovingPoint.reset();
      }
    }

    _isFinalized = true;
    return _accessMap;
  }
  catch(std::exception& error)
  {
    Log::error() << "ImageAccessWatcher::finalize: unable to finalize watcher: " << error.what();
    MT_ASSERT(false && "Unable to finalize watcher");
  }
}
