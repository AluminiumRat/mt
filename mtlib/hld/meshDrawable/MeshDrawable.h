#pragma once

#include <glm/glm.hpp>

#include <hld/drawScene/Drawable.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <util/Ref.h>

namespace mt
{
  class MeshDrawable : public Drawable
  {
  public:
    MeshDrawable();
    MeshDrawable(const MeshDrawable&) = delete;
    MeshDrawable& operator = (const MeshDrawable&) = delete;
    virtual ~MeshDrawable() noexcept = default;

    inline MeshAsset* asset() const noexcept;
    virtual void setAsset(MeshAsset* newAsset);

    virtual void addToDrawPlan( DrawPlan& plan,
                                uint32_t frameTypeIndex) const override;

    virtual void addToCommandList(
                              DrawCommandList& commandList,
                              const FrameContext& frameContext) const override;

  private:
    Ref<MeshAsset> _asset;
  };

  inline MeshAsset* MeshDrawable::asset() const noexcept
  {
    return _asset.get();
  }
}