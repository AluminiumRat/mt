#pragma once

#include <algorithm>
#include <vector>

#include <hld/drawScene/Drawable.h>
#include <hld/drawScene/Drawable3D.h>
#include <hld/drawScene/DrawScene.h>
#include <util/Assert.h>

namespace mt
{
  class DrawPlan;
  class Camera;

  class DrawScene
  {
  public:
    DrawScene() noexcept = default;
    DrawScene(const Drawable&) = delete;
    DrawScene& operator = (const Drawable&) = delete;
    virtual ~DrawScene() noexcept = default;

    inline void registerDrawable(Drawable& drawable);
    inline void unregisterDrawable(Drawable& drawable);

    virtual void fillDrawPlan(DrawPlan& plan,
                              const Camera& camera,
                              FrameTypeIndex frameTypeIndex) const;

  private:
    using Drawables = std::vector<Drawable*>;
    Drawables _drawables;

    using Drawables3D = std::vector<Drawable3D*>;
    Drawables3D _drawables3D;
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
}