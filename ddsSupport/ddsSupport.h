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
};