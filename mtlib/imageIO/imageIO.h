#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>

#include <vulkan/vulkan.hpp>

#include <util/Ref.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/image/Image.h>

namespace mt
{
  class Device;
  class CommandQueueTransfer;

  //  Загрузить изображение из файла
  //  Загрузчик определяется по расширению файла, будет использован либо
  //    loadDDS для *.dds файлов, либо loadStbLDR/loadStbHDR для остальных
  //    расширений
  //  uploadQueue - это очередь, через которую будет производится
  //    загрузка текстуры и создание мипов(при необходимости).
  //  layoutAutocontrol - надо ли включать автоконтроль лэйаута у создаваемого
  //    image. Если layoutAutocontrol отключен, то image после загрузки будет
  //    переведен в лэйаут VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  //  Обратите внимание, метод отправляет необходимые команды в очередь, но не
  //    ожидает их выполнения. Если вы используете image в очереди, отличной от
  //    transferQueue, то необходима внешняя синхронизация. Впрочем,
  //    CommandQueue::ownershipTransfer и так её делает
  inline Ref<Image> loadImage(const std::filesystem::path& file,
                              CommandQueueGraphic& uploadQueue,
                              bool layoutAutocontrol = false);

  //  Загрузить ldr изображение с помошью библиотеки stb. Используется
  //    для форматов файлов bmp, jpg, png
  //  Форматы итогового изображения VK_FORMAT_R8G8B8A8_SRGB
  //  После загрузки будут автоматически сгенерированы мипы
  //  uploadQueue - это очередь, через которую будет производится
  //    загрузка текстуры и создание мипов(при необходимости).
  //  layoutAutocontrol - надо ли включать автоконтроль лэйаута у создаваемого
  //    image. Если layoutAutocontrol отключен, то image после загрузки будет
  //    переведен в лэйаут VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  //  Обратите внимание, метод отправляет необходимые команды в очередь, но не
  //    ожидает их выполнения. Если вы используете image в очереди, отличной от
  //    transferQueue, то необходима внешняя синхронизация. Впрочем,
  //    CommandQueue::ownershipTransfer и так её делает
  Ref<Image> loadStbLDR(const std::filesystem::path& file,
                        CommandQueueGraphic& uploadQueue,
                        bool layoutAutocontrol = false);

  //  Загрузить hdr изображение с помошью библиотеки stb. Используется
  //    для формата hdr
  //  Форматы итогового изображения VK_FORMAT_R32G32B32A32_SFLOAT
  //  После загрузки будут автоматически сгенерированы мипы
  //  uploadQueue - это очередь, через которую будет производится
  //    загрузка текстуры и создание мипов(при необходимости).
  //  layoutAutocontrol - надо ли включать автоконтроль лэйаута у создаваемого
  //    image. Если layoutAutocontrol отключен, то image после загрузки будет
  //    переведен в лэйаут VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  //  Обратите внимание, метод отправляет необходимые команды в очередь, но не
  //    ожидает их выполнения. Если вы используете image в очереди, отличной от
  //    transferQueue, то необходима внешняя синхронизация. Впрочем,
  //    CommandQueue::ownershipTransfer и так её делает
  Ref<Image> loadStbHDR(const std::filesystem::path& file,
                        CommandQueueGraphic& uploadQueue,
                        bool layoutAutocontrol = false);

  //  Загрузить dds файл, используя библиотеку dds_image
  //  Данные загружаются как есть. Тип картинки, формат, размер массива и
  //    количество мип уровней полностью определяются содержимым файла
  //  transferQueue - это очередь, через которую будет производится
  //    загрузка текстуры. Может быть nullptr, тогда текстура будет прогружаться
  //    через device.primaryQueue
  //  layoutAutocontrol - надо ли включать автоконтроль лэйаута у создаваемого
  //    image. Если layoutAutocontrol отключен, то image после загрузки будет
  //    переведен в лэйаут VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  //  ВНИМАНИЕ! transferQueue захватит владение image-ем. Если вы используете
  //    image в очереди, отличной от transferQueue, то необходимо произвести
  //    трансфер владения (CommandQueue::ownershipTransfer)
  //  Обратите внимание, метод отправляет необходимые команды в очередь, но не
  //    ожидает их выполнения. Если вы используете image в очереди, отличной от
  //    transferQueue, то необходима внешняя синхронизация. Впрочем,
  //    CommandQueue::ownershipTransfer и так её делает
  Ref<Image> loadDDS( const std::filesystem::path& file,
                      Device& device,
                      CommandQueueTransfer* transferQueue = nullptr,
                      bool layoutAutocontrol = false);

  //  Скачать srcImage с ГПУ и сохранить его в виде dds файла
  //  transferQueue - это очередь, через которую будет производится
  //    выгрузка текстуры. Может быть nullptr, тогда текстура будет прогружаться
  //    через device.primaryQueue
  //  ВНИМАНИЕ! Если transferque отличается от очереди, в которой использовалась
  //    srcImage, то необходимо произвести трансфер владения (
  //    CommandQueue::ownershipTransfer)
  //  ВНИМАНИЕ! Функция предполагает, что у srcImage включен автоконтроль
  //    лэйаутов, либо он находится в лэйауте
  //    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
  //  ВНИМАНИЕ! srcImage должен быть создан со включенным флагом
  //    VK_IMAGE_USAGE_TRANSFER_SRC_BIT
  void saveToDDS( const Image& srcImage,
                  const std::filesystem::path& file,
                  CommandQueueTransfer* transferQueue = nullptr);

  //  Реализайия функции
  inline Ref<Image> loadImage(const std::filesystem::path& file,
                              CommandQueueGraphic& uploadQueue,
                              bool layoutAutocontrol)
  {
    std::string extension = file.extension().string();
    std::transform( extension.begin(),
                    extension.end(),
                    extension.begin(),
                    [](unsigned char c){return std::tolower(c);});
    if(extension == ".dds")
    {
      return loadDDS( file,
                      uploadQueue.device(),
                      &uploadQueue,
                      layoutAutocontrol);
    }
    else if(extension == ".hdr")
    {
      return loadStbHDR(file, uploadQueue, layoutAutocontrol);
    }
    else
    {
      return loadStbLDR(file, uploadQueue, layoutAutocontrol);
    }
  }
};