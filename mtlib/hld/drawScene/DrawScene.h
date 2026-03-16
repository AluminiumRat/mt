#pragma once

#include <algorithm>
#include <vector>

#include <hld/drawScene/Drawable.h>
#include <hld/drawScene/Drawable3D.h>
#include <hld/drawScene/DrawScene.h>
#include <util/Assert.h>
#include <vkr/accelerationStructure/TLAS.h>

namespace mt
{
  class Camera;
  class CommandProducerCompute;
  class DrawPlan;
  struct FrameBuildContext;

  class DrawScene
  {
  public:
    DrawScene() noexcept;
    DrawScene(const Drawable&) = delete;
    DrawScene& operator = (const Drawable&) = delete;
    virtual ~DrawScene() noexcept = default;

    inline void registerDrawable(Drawable& drawable);
    inline void unregisterDrawable(Drawable& drawable);

    virtual void fillDrawPlan(DrawPlan& plan,
                              const FrameBuildContext& frameContext) const;

    inline void registerBLAS(BLASInstance& blas);
    inline void unregisterBLAS(BLASInstance& blas);
    inline void notifyBLASMoved(BLASInstance& blas);

    //  Актуальная TLAS появится только после вызова updateGPUData
    inline const TLAS* tlas() const noexcept;

    //  Необходимо вызывать каждый кадр после апдэйта всех данных на CPU
    virtual void updateGPUData(CommandProducerCompute& commandProducer);

  private:
    using Drawables = std::vector<Drawable*>;
    Drawables _drawables;

    using Drawables3D = std::vector<Drawable3D*>;
    Drawables3D _drawables3D;

    using BLASList = std::vector<BLASInstance*>;
    BLASList _blasList;

    Ref<TLAS> _tlas;
    bool _needUpdateTLAS;
    bool _needRebuildTLAS;
  };

  inline void DrawScene::registerDrawable(Drawable& drawable)
  {
    Drawable3D* drawable3D = drawable.asDrawable3D();
    if(drawable3D != nullptr)
    {
      MT_ASSERT(std::find(_drawables3D.begin(), _drawables3D.end(), drawable3D) == _drawables3D.end());
      _drawables3D.push_back(drawable3D);
    }
    else
    {
      MT_ASSERT(std::find(_drawables.begin(), _drawables.end(), &drawable) == _drawables.end());
      _drawables.push_back(&drawable);
    }
  }

  inline void DrawScene::unregisterDrawable(Drawable& drawable)
  {
    Drawable3D* drawable3D = drawable.asDrawable3D();
    if(drawable3D != nullptr)
    {
      Drawables3D::iterator iDrawable = std::find(_drawables3D.begin(),
                                                  _drawables3D.end(),
                                                  drawable3D);
      if(iDrawable != _drawables3D.end()) _drawables3D.erase(iDrawable);
    }
    else
    {
      Drawables::iterator iDrawable = std::find(_drawables.begin(),
                                                _drawables.end(),
                                                &drawable);
      if(iDrawable != _drawables.end()) _drawables.erase(iDrawable);
    }
  }

  inline void DrawScene::registerBLAS(BLASInstance& blas)
  {
    MT_ASSERT(std::find(_blasList.begin(), _blasList.end(), &blas) == _blasList.end());
    _blasList.push_back(&blas);
    _needRebuildTLAS = true;
  }

  inline void DrawScene::unregisterBLAS(BLASInstance& blas)
  {
    BLASList::iterator iBLAS = std::find( _blasList.begin(),
                                          _blasList.end(),
                                          &blas);
    if(iBLAS != _blasList.end())
    {
      _blasList.erase(iBLAS);
      _needRebuildTLAS = true;
    }
  }

  inline void DrawScene::notifyBLASMoved(BLASInstance& blas)
  {
    _needUpdateTLAS = true;
  }

  inline const TLAS* DrawScene::tlas() const noexcept
  {
    return _tlas.get();
  }
}