#pragma once

#include <vulkan/vulkan.hpp>

#include <util/Ref.h>
#include <vkr/image/Image.h>

namespace mt
{
  class Device;
  class CommandQueueTransfer;

  //  Загрузить dds файл
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
  Ref<Image> loadDDS( const char* filename,
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
                  const char* filename,
                  CommandQueueTransfer* transferQueue = nullptr);
};