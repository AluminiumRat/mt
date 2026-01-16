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

    inline const MeshAsset* asset() const noexcept;
    virtual void setAsset(const MeshAsset* newAsset);

    virtual void addToDrawPlan( DrawPlan& plan,
                                uint32_t frameTypeIndex) const override;

    virtual void addToCommandList(
                              DrawCommandList& commandList,
                              const FrameContext& frameContext) const override;

  private:
    ConstRef<MeshAsset> _asset;
  };

  inline const MeshAsset* MeshDrawable::asset() const noexcept
  {
    return _asset.get();
  }
}