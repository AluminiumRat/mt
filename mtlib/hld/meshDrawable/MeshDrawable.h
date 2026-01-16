#pragma once

#include <glm/glm.hpp>

#include <hld/drawScene/Drawable.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <util/Ref.h>

namespace mt
{
  //  Базовый класс дравэйбла-меша
  //  Меш - некоторая геометрия, задаваемая точками, которая отрисовывается
  //    одной техникой
  //  Есть 2 вида объектов для отрисовки меша:
  //    Меш ассет(MeshAsset) - это разделяемый ресурс, который хранит технику
  //      отрисовки и все общие данные
  //    Меш дравэйбл(MeshDrawable) - конкретный экземпляр на сцене. Отвечает
  //      за индивидуальные данные (положение, тинт и т.д)
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