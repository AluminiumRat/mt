#pragma once

#include <algorithm>
#include <vector>

#include <hld/drawScene/Drawable.h>
#include <hld/drawScene/Drawable3D.h>
#include <hld/drawScene/DrawScene.h>
#include <util/Assert.h>

namespace mt
{
  //  Простейший вариант сцены
  class SimpleDrawScene : public DrawScene
  {
  public:
    SimpleDrawScene() noexcept = default;
    SimpleDrawScene(const Drawable&) = delete;
    SimpleDrawScene& operator = (const Drawable&) = delete;
    virtual ~SimpleDrawScene() noexcept = default;

    inline void addDrawable(Drawable& drawable);
    inline void removeDrawable(Drawable& drawable);

    virtual void fillDrawPlan(DrawPlan& plan,
                              Camera& camera,
                              FrameTypeIndex frameTypeIndex) const override;

  private:
    using Drawables = std::vector<Drawable*>;
    Drawables _drawables;

    using Drawables3D = std::vector<Drawable3D*>;
    Drawables3D _drawables3D;
  };

  inline void SimpleDrawScene::addDrawable(Drawable& drawable)
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

  inline void SimpleDrawScene::removeDrawable(Drawable& drawable)
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
}