#include <stdexcept>

#include <ddsSupport/ddsSupport.h>
#include <resourceManagement/TextureManager.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/image/Image.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/Device.h>

namespace fs = std::filesystem;

using namespace mt;

static VkImageViewType getViewType(const Image& image, const fs::path& filePath)
{
  switch(image.imageType())
  {
  case VK_IMAGE_TYPE_1D:
    if(image.arraySize() == 1) return VK_IMAGE_VIEW_TYPE_1D;
    else return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
  case VK_IMAGE_TYPE_2D:
    if (image.arraySize() == 1) return VK_IMAGE_VIEW_TYPE_2D;
    else
    {
      std::string filename = (const char*)filePath.stem().u8string().c_str();
      if(filename.find("CUBEMAP") != std::string::npos)
      {
        if(image.arraySize() % 6 != 0)
        {
          std::string filepathStr = (const char*)filePath.u8string().c_str();
          Log::error() << "Texture manager: texture " << filePath << " has array size " << image.arraySize() << " but it must be cubemap";
          throw std::runtime_error(filepathStr + ": wrong array size");
        }
        if(image.arraySize() == 6) return VK_IMAGE_VIEW_TYPE_CUBE;
        else return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
      }
      return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
  case VK_IMAGE_TYPE_3D:
    return VK_IMAGE_VIEW_TYPE_3D;
  default:
    Abort("Unknown image type");
  }
}

static Ref<ImageView> loadFromDDS(const fs::path& filePath, 
                                  CommandQueueTransfer& ownerQueue)
{
  try
  {
    Ref<Image> loadedImage = loadDDS( filePath,
                                      ownerQueue.device(),
                                      &ownerQueue);
    return Ref(new ImageView( *loadedImage,
                              ImageSlice(*loadedImage),
                              getViewType(*loadedImage, filePath)));
  }
  catch(std::exception& error)
  {
    Log::error() << "TextureManager: unable to load: " << filePath << " : " << error.what();
    return Ref<ImageView>();
  }
}

class TextureManager::LoadTextureTask : public AsyncTask
{
public:
  LoadTextureTask(const fs::path& filePath,
                  CommandQueueTransfer& ownerQueue,
                  TextureManager& manager) :
    AsyncTask((const char*)filePath.u8string().c_str(),
              AsyncTask::EXCLUSIVE_MODE,
              AsyncTask::SILENT),
    _filePath(filePath),
    _ownerQueue(ownerQueue),
    _manager(manager)
  {
  }

  virtual void asyncPart() override
  {
    AsyncTask::asyncPart();

    Ref<Image> loadedImage = loadDDS(_filePath,
                                      _ownerQueue.device(),
                                      &_ownerQueue);
    _loadedImage =  Ref(new ImageView(*loadedImage,
                                      ImageSlice(*loadedImage),
                                      getViewType(*loadedImage, _filePath)));
  }

  virtual void finalizePart() override
  {
    MT_ASSERT(_loadedImage != nullptr);
    AsyncTask::finalizePart();
    _manager._updateTexture(_filePath, _ownerQueue, *_loadedImage);
  }

private:
  fs::path _filePath;
  CommandQueueTransfer& _ownerQueue;
  TextureManager& _manager;
  ConstRef<ImageView> _loadedImage;
};

TextureManager::TextureManager( FileWatcher& fileWatcher,
                                AsyncTaskQueue& loadingQueue) :
  _fileWatcher(fileWatcher),
  _loadingQueue(loadingQueue)
{
}

TextureManager::~TextureManager() noexcept
{
  _fileWatcher.removeObserver(*this);
}

static Ref<ImageView> createDefaultImage(CommandQueueTransfer& ownerQueue)
{
  Device& device = ownerQueue.device();

  uint32_t pixels[16] = { 0xFF0030FF, 0xFF000000, 0xFF0030FF, 0xFF000000,
                          0xFF000000, 0xFF0030FF, 0xFF000000, 0xFF0030FF,
                          0xFF0030FF, 0xFF000000, 0xFF0030FF, 0xFF000000,
                          0xFF000000, 0xFF0030FF, 0xFF000000, 0xFF0030FF };
  Ref<DataBuffer> uploadingBuffer(new DataBuffer( device,
                                                  sizeof(pixels),
                                                  DataBuffer::UPLOADING_BUFFER,
                                                  "Uploading buffer"));
  uploadingBuffer->uploadData(pixels, 0, sizeof(pixels));

  Ref<Image> image(new Image( device,
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              0,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              glm::uvec3(4, 4, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              false,
                              "Default texture"));

  std::unique_ptr<CommandProducerTransfer> producer =
                                                    ownerQueue.startCommands();

  producer->imageBarrier( *image,
                          ImageSlice(*image),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          0,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0,
                          0);

  producer->copyFromBufferToImage(*uploadingBuffer,
                                  0,
                                  4,
                                  4,
                                  *image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  0,
                                  1,
                                  0,
                                  glm::uvec3(0,0,0),
                                  glm::uvec3(4,4,1));

  producer->imageBarrier( *image,
                          ImageSlice(*image),
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                            VK_ACCESS_SHADER_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

  ownerQueue.submitCommands(std::move(producer));

  return Ref(new ImageView( *image,
                            ImageSlice(*image),
                            VK_IMAGE_VIEW_TYPE_2D));
}

TechniqueResource* TextureManager::_getExistingResource(
                                              const fs::path& filePath,
                                              CommandQueueTransfer& ownerQueue,
                                              bool useDefaultTexture) const
{
  ResourcesMap::const_iterator iResource =
                                  _resourcesMap.find({ filePath, &ownerQueue });
  if(iResource == _resourcesMap.end()) return nullptr;

  if(useDefaultTexture) return iResource->second.withDefault.get();
  else return iResource->second.noDefault.get();
}

ConstRef<TechniqueResource> TextureManager::loadImmediately(
                                              const fs::path& filePath,
                                              CommandQueueTransfer& ownerQueue,
                                              bool useDefaultTexture)
{
  fs::path normalizedPath = filePath.lexically_normal();

  {
    // Первичная проверка, не загружен ли уже ресурс
    std::lock_guard lock(_accessMutex);
    const TechniqueResource* resource = _getExistingResource(
                                                            normalizedPath,
                                                            ownerQueue,
                                                            useDefaultTexture);
    if(resource != nullptr) return ConstRef(resource);
  }

  //  Если ресурс ещё не был загружен, загружаем его
  Ref<ImageView> image = loadFromDDS(normalizedPath, ownerQueue);

  //  Повторная проверка, может кто-то ещё успел загрузить ресурс пока мы читали
  //  dds
  std::lock_guard lock(_accessMutex);
  TechniqueResource* resource = _getExistingResource( normalizedPath,
                                                      ownerQueue,
                                                      useDefaultTexture);
  if(resource != nullptr) return ConstRef(resource);

  //  Убедились, что ресурса точно нет в мапе, создадим новый
  ResourceRecord newRecord;
  newRecord.withDefault = Ref(new TechniqueResource);
  newRecord.noDefault = Ref(new TechniqueResource);

  if (image != nullptr)
  {
    newRecord.withDefault->setImage(image.get());
    newRecord.noDefault->setImage(image.get());
  }
  else
  {
    // Текстура не загрузилась - используем вместо неё дефлотную
    image = createDefaultImage(ownerQueue);
    newRecord.withDefault->setImage(image.get());
  }

  resource = useDefaultTexture ? newRecord.withDefault.get() :
                                 newRecord.noDefault.get();
  _resourcesMap[{normalizedPath, & ownerQueue}] = std::move(newRecord);

  _addFileWatching(normalizedPath);

  return ConstRef(resource);
}

void TextureManager::_addFileWatching(const fs::path& filePath) noexcept
{
  try
  {
    _fileWatcher.addWatching(filePath, *this);
  }
  catch(std::exception& error)
  {
    Log::error() << "TextureManager: unable to add file for watching: " << filePath << " : " << error.what();
  }
}

ConstRef<TechniqueResource> TextureManager::sheduleLoading(
                                          const std::filesystem::path& filePath,
                                          CommandQueueTransfer& ownerQueue,
                                          bool useDefaultTexture)
{
  fs::path normalizedPath = filePath.lexically_normal();

  //  Проверка, не загружен ли уже ресурс
  std::lock_guard lock(_accessMutex);
  const TechniqueResource* resource = _getExistingResource(
                                                          normalizedPath,
                                                          ownerQueue,
                                                          useDefaultTexture);
  if(resource != nullptr) return ConstRef(resource);

  //  Если ресурс ещё не был загружен ранее, то создаем новую запись в таблице
  //  ресурсов и отправляем задачу на загрузку
  ResourceRecord newRecord;
  newRecord.noDefault = Ref(new TechniqueResource);
  newRecord.withDefault = Ref(new TechniqueResource);
  newRecord.withDefault->setImage(createDefaultImage(ownerQueue).get());

  std::unique_ptr<LoadTextureTask> loadTask(new LoadTextureTask(normalizedPath,
                                                                ownerQueue,
                                                                *this));
  newRecord.loadingHandle = _loadingQueue.addManagedTask(std::move(loadTask));

  _fileWatcher.addWatching(normalizedPath, *this);

  resource = useDefaultTexture ?  newRecord.withDefault.get() :
                                  newRecord.noDefault.get();
  _resourcesMap[{normalizedPath, & ownerQueue}] = std::move(newRecord);

  _addFileWatching(normalizedPath);

  return ConstRef(resource);
}

void TextureManager::onFileChanged( const fs::path& filePatch,
                                    EventType eventType)
{
  if(eventType == FILE_DISAPPEARANCE) return;

  std::lock_guard lock(_accessMutex);

  //  Если файл изменился или вновь появился, то запускаем его перезагрузку
  //  в фоновом режиме
  for(ResourcesMap::iterator iResource = _resourcesMap.begin();
      iResource != _resourcesMap.end();
      iResource++)
  {
    const ResourcesKey& key = iResource->first;
    if(key.filePath == filePatch)
    {
      ResourceRecord& record = iResource->second;
      std::unique_ptr<LoadTextureTask> loadTask(new LoadTextureTask(
                                                              filePatch,
                                                              *key.commandQueue,
                                                              *this));
      record.loadingHandle = _loadingQueue.addManagedTask(std::move(loadTask));
    }
  }
}

void TextureManager::_updateTexture(const fs::path& filePath,
                                    CommandQueueTransfer& ownerQueue,
                                    const ImageView& imageView)
{
  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource =
                                    _resourcesMap.find({filePath, &ownerQueue});
  MT_ASSERT(iResource != _resourcesMap.end());
  iResource->second.withDefault->setImage(&imageView);
  iResource->second.noDefault->setImage(&imageView);
}

int TextureManager::_getRecordsCount(const fs::path& filePath) const noexcept
{
  int count = 0;
  for(ResourcesMap::const_iterator iResource = _resourcesMap.begin();
      iResource != _resourcesMap.end();
      iResource++)
  {
    if(iResource->first.filePath == filePath) count++;
  }
  return count;
}

void TextureManager::removeUnused() noexcept
{
  std::lock_guard lock(_accessMutex);

  //  Ищем ресурсы, на которые нет внешних ссылок
  ResourcesMap::iterator iResource = _resourcesMap.begin();
  while(iResource != _resourcesMap.end())
  {
    const ResourcesKey& key = iResource->first;
    ResourceRecord& record = iResource->second;

    if( record.withDefault->counterValue() == 1 &&
        record.noDefault->counterValue() == 1)
    {
      //  Ищем другие ресурсы, ссылающиеся на тот же файл, чтобы понять, надо ли
      //  прекращать слежение за файлом
      if(_getRecordsCount(key.filePath) == 1)
      {
        _fileWatcher.removeWatching(key.filePath, *this);
      }
      iResource = _resourcesMap.erase(iResource);
    }
    else iResource++;
  }
}
