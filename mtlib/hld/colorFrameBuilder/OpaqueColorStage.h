#pragma once

#include <hld/RegularDrawStage.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class OpaqueColorStage : public RegularDrawStage
  {
  public:
    static constexpr const char* stageName = "OpaqueColorStage";

  public:
    explicit OpaqueColorStage(Device& device);
    OpaqueColorStage(const OpaqueColorStage&) = delete;
    OpaqueColorStage& operator = (const OpaqueColorStage&) = delete;
    virtual ~OpaqueColorStage() noexcept = default;

    inline void setBuffers( const ImageView& hdrBuffer,
                            const ImageView& depthBuffer);

  private:
    virtual ConstRef<FrameBuffer> buildFrameBuffer() const override;

  private:
    ConstRef<ImageView> _hdrBuffer;
    ConstRef<ImageView> _depthBuffer;
  };

  inline void OpaqueColorStage::setBuffers(
                                          const ImageView& hdrBuffer,
                                          const ImageView& depthBuffer)
  {
    if(_hdrBuffer == &hdrBuffer && _depthBuffer == &depthBuffer) return;
    _hdrBuffer = &hdrBuffer;
    _depthBuffer = &depthBuffer;
    resetFrameBuffer();
  }
}