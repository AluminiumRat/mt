#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/ImageAccessWatcher.h>
#include <vkr/Image.h>

using namespace mt;

ImageAccessWatcher::ImageAccessWatcher() noexcept :
  _accessMap(1087),
  _isFinalized(false)
{
}

ImageAccessWatcher::MemoryConflict ImageAccessWatcher::addImageAccess(
                                        const Image& image,
                                        const ImageAccess& newAccess,
                                        CommandBuffer& matchingBuffer) noexcept
{
  MT_ASSERT(!_isFinalized);

  try
  {
    auto insertion = _accessMap.emplace(&image, ImageAccessState());
    bool isNewRecord = insertion.second;
    ImageAccessState& imageState = insertion.first->second;

    if(isNewRecord)
    {
      //  Image ещё не использовался в текущей сессии
      imageState.currentAccess = newAccess;
      return NO_MEMORY_CONFLICT;
    }

    ImageAccess& currentAccess = imageState.currentAccess;

    //  Для начала пытаемся поженить предыдущий и следующий доступы без барьеров
    if(currentAccess.mergeNoBarriers(newAccess))
    {
      return NO_MEMORY_CONFLICT;
    }

    //  Без барьера объединить не получилось
    //  Для начала резолвим предыдущую точку согласования
    if (imageState.lastMatchingPoint.has_value())
    {
      imageState.lastMatchingPoint->makeMatch(image, currentAccess);
      imageState.lastMatchingPoint.reset();
    }
    else
    {
      //  Точки согласования нет, значит это был первый доступ в сессии,
      //  сохраним его
      imageState.initialAccess = currentAccess;
    }

    //  Готовим новую точку согласования между текущим доступом и следующим
    //  transformHint здесь временный, мы его пока ещё не знаем
    imageState.lastMatchingPoint = MatchingPoint{
                                .matchingBuffer = &matchingBuffer,
                                .previousAccess = imageState.currentAccess,
                                .transformHint = ImageAccess::RESET_SLICES };

    //  Из текущего доступа и нового пришедшего изобретаем новое состояние
    //  лэйаутов так, чтобы преобразовывать потребовалось поменьше
    imageState.lastMatchingPoint->transformHint =
                          imageState.currentAccess.mergeWithBarriers(newAccess);

    return NEED_TO_MATCHING;
  }
  catch (std::exception& error)
  {
    Log::error() << "ImageAccessWatcher::addImageAccess: unable to add image access: " << error.what();
    Abort("Unable to add image access");
  }
}

const ImageAccessMap& ImageAccessWatcher::finalize() noexcept
{
  // Выставить initailAcces у всех image, у кого ещё не выставлен
  // Закрыть все MatchingPoint-ы
  try
  {
    for(ImageAccessMap::iterator iImageState = _accessMap.begin();
        iImageState != _accessMap.end();
        iImageState++)
    {
      const Image* image = iImageState->first;
      ImageAccessState& imageState = iImageState->second;

      if(!imageState.initialAccess.has_value())
      {
        imageState.initialAccess = imageState.currentAccess;
      }

      if(imageState.lastMatchingPoint.has_value())
      {
        imageState.lastMatchingPoint->makeMatch(*image,
                                                imageState.currentAccess);
        imageState.lastMatchingPoint.reset();
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
