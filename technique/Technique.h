#pragma once

#include <string>

#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueConfigurator.h>
#include <util/Ref.h>
#include <util/RefCounter.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  class Technique : public RefCounter
  {
  public:
    Technique(Device& device, const char* debugName = "Technique");
    Technique(const Technique&) = delete;
    Technique& operator = (const Technique&) = delete;
  protected:
    virtual ~Technique() noexcept = default;
  public:

    inline Device& device() const noexcept;

    inline TechniqueConfigurator& configurator() const noexcept;

    //  Подключить пайплайн и все необходимые ресурсы к продюсеро
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько техник к одному
    //    продюсеру.
    bool bindGraphic(CommandProducerGraphic& producer);
    void unbindGraphic(CommandProducerGraphic& producer);

  private:
    void _checkConfiguration();
    void _applyToConfiguration();

  private:
    Device& _device;
    std::string _debugName;
    bool _isBinded;
    Ref<TechniqueConfigurator> _configurator;
    Ref<TechniqueConfiguration> _configuration;
    size_t _lastConfiguratorRevision;
  };

  inline Device& Technique::device() const noexcept
  {
    return _device;
  }

  inline TechniqueConfigurator& Technique::configurator() const noexcept
  {
    return *_configurator;
  }
}