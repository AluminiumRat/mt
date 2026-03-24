#pragma once

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class ImageView;

  //  Строит velocity буфер по линейному буферу глубини и информации о
  //  перемещении камеры
  class VelocityBufferUpdater
  {
  public:
    explicit VelocityBufferUpdater(Device& device);
    VelocityBufferUpdater(const VelocityBufferUpdater&) = delete;
    VelocityBufferUpdater& operator = (const VelocityBufferUpdater&) = delete;
    ~VelocityBufferUpdater() noexcept = default;

    //  К моменту вызова velocityBuffer должен находиться в лэйауте
    //    VK_IMAGE_LAYOUT_GENERAL
    void updateVelocityBuffer(CommandProducerGraphic& commandProducer,
                              const ImageView& velocityBuffer);
  private:
    Ref<TechniqueConfigurator> _techniqueConfigurator;
    Technique _technique;
    TechniquePass& _pass;
    ResourceBinding& _velocityBufferBinding;
  };
}