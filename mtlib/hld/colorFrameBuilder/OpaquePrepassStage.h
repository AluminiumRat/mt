#pragma once

#include <hld/RegularDrawStage.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  //  Создание depth/stencil, normal и roughtness буферов
  class OpaquePrepassStage : public RegularDrawStage
  {
  public:
    static constexpr const char* stageName = "OpaquePrepass";

  public:
    explicit OpaquePrepassStage(Device& device);
    OpaquePrepassStage(const OpaquePrepassStage&) = delete;
    OpaquePrepassStage& operator = (const OpaquePrepassStage&) = delete;
    virtual ~OpaquePrepassStage() noexcept = default;

    inline void setBuffer(const ImageView& depthBuffer);

  protected:
    virtual ConstRef<FrameBuffer> buildFrameBuffer() const override;

  private:
    ConstRef<ImageView> _depthBuffer;
  };

  inline void OpaquePrepassStage::setBuffer(const ImageView& depthBuffer)
  {
    if(_depthBuffer == &depthBuffer) return;
    _depthBuffer = &depthBuffer;
    resetFrameBuffer();
  }
}