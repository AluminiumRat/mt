#pragma once

#include <hld/RegularDrawStage.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  //  Создание depth/stencil, linear depth и normal буферов
  class OpaquePrepassStage : public RegularDrawStage
  {
  public:
    static constexpr const char* stageName = "OpaquePrepass";

  public:
    explicit OpaquePrepassStage(Device& device);
    OpaquePrepassStage(const OpaquePrepassStage&) = delete;
    OpaquePrepassStage& operator = (const OpaquePrepassStage&) = delete;
    virtual ~OpaquePrepassStage() noexcept = default;

    inline void setBuffers( const ImageView& depthBuffer,
                            const ImageView& linearDepthBuffer,
                            const ImageView& normalBuffer);

  protected:
    virtual ConstRef<FrameBuffer> buildFrameBuffer() const override;

  private:
    ConstRef<ImageView> _depthBuffer;
    ConstRef<ImageView> _linearDepthBuffer;
    ConstRef<ImageView> _normalBuffer;
  };

  inline void OpaquePrepassStage::setBuffers(
                                            const ImageView& depthBuffer,
                                            const ImageView& linearDepthBuffer,
                                            const ImageView& normalBuffer)
  {
    if( _depthBuffer == &depthBuffer &&
        _linearDepthBuffer == &linearDepthBuffer &&
        _normalBuffer == &normalBuffer) return;
    _depthBuffer = &depthBuffer;
    _linearDepthBuffer = &linearDepthBuffer;
    _normalBuffer = &normalBuffer;
    resetFrameBuffer();
  }
}