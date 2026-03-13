#pragma once

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class DrawPlan;
  struct FrameBuildContext;

  //  Базовый класс для стадий отрисовки, которые просто транслируют DrawPlan
  //  в отрисовку во фрэйм буффер
  class RegularDrawStage
  {
  public:
    RegularDrawStage( Device& device,
                      const char* stageName,
                      DrawCommandList::Sorting sorting);
    RegularDrawStage(const RegularDrawStage&) = delete;
    RegularDrawStage& operator = (const RegularDrawStage&) = delete;
    virtual ~RegularDrawStage() noexcept = default;

    virtual void draw(CommandProducerGraphic& commandProducer,
                      const DrawPlan& drawPlan,
                      const FrameBuildContext& frameContext,
                      glm::uvec2 viewport);

    inline Device& device() const noexcept;

  protected:
    //  Сигнализировать о том, что фрэйм буффер необходимо пересобрать. Нужно
    //  вызывать при смене таргетов
    inline void resetFrameBuffer() noexcept;

    //  Вызывается из метода draw, когда нужно создать фрэйм буфер, в который
    //  производится отрисовка
    virtual ConstRef<FrameBuffer> buildFrameBuffer() const = 0;

  private:
    Device& _device;
    StageIndex _stageIndex;
    ConstRef<FrameBuffer> _frameBuffer;
    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;
    DrawCommandList::Sorting _sorting;
  };

  inline Device& RegularDrawStage::device() const noexcept
  {
    return _device;
  }

  inline void RegularDrawStage::resetFrameBuffer() noexcept
  {
    _frameBuffer.reset();
  }
}