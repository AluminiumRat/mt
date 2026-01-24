#pragma once

#include <glm/glm.hpp>

#include <hld/drawScene/Drawable3D.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <util/Ref.h>
#include <util/Signal.h>

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
  class MeshDrawable : public Drawable3D
  {
  public:
    explicit MeshDrawable(const MeshAsset& asset);
    MeshDrawable(const MeshDrawable&) = delete;
    MeshDrawable& operator = (const MeshDrawable&) = delete;
    virtual ~MeshDrawable() noexcept;

    inline const MeshAsset& asset() const noexcept;

    virtual void addToDrawPlan( DrawPlan& plan,
                                FrameTypeIndex frameTypeIndex) const override;

    virtual void addToCommandList(DrawCommandList& commandList,
                                  FrameTypeIndex frame,
                                  StageIndex stage,
                                  const void* extraData) const override;

    //  Матрица положения в текущем кадре
    inline const glm::mat4& positionMatrix() const noexcept;
    virtual void setPositionMatrix(const glm::mat4& newValue);

    //  Матрица положения в предыдущем кадре
    inline const glm::mat4& prevPositionMatrix() const noexcept;
    void setPrevPositionMatrix(const glm::mat4& newValue);

    //  Матрица преобразования нормалей (и прочих бивекторов)
    //  Автоматически вычисляется на основе positionMatrix, если она
    //    используется в технике отрисовки
    inline const glm::mat3x3& bivecMatrix() const noexcept;

  protected:
    virtual void onAssetUpdated();

  private:
    void _updateBivecMatrix() noexcept;
    void _updateBoundingBox() noexcept;

  private:
    ConstRef<MeshAsset> _asset;
    Slot<> _onAssetUpdatedSlot;

    //  Матрица положения в текущем кадре
    glm::mat4 _positionMatrix;
    //  Матрица положения в предыдущем кадре
    glm::mat4 _prevPositionMatrix;
    //  Матрица преобразования нормалей (и прочих бивекторов)
    glm::mat3x3 _bivecMatrix;
  };

  inline const MeshAsset& MeshDrawable::asset() const noexcept
  {
    return *_asset;
  }

  inline const glm::mat4& MeshDrawable::positionMatrix() const noexcept
  {
    return _positionMatrix;
  }

  inline const glm::mat4& MeshDrawable::prevPositionMatrix() const noexcept
  {
    return _prevPositionMatrix;
  }

  inline const glm::mat3x3& MeshDrawable::bivecMatrix() const noexcept
  {
    return _bivecMatrix;
  }
}