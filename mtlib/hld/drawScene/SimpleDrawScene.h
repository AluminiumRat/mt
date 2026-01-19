#pragma once

#include <algorithm>
#include <vector>

#include <hld/drawScene/DrawScene.h>
#include <util/Assert.h>

namespace mt
{
  class Drawable;

  //  Простейший вариант сцены. Хранит ссылки на Drawable, отправляет
  //  их на рендер всем скопом без frustum culling-а
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
  };

  inline void SimpleDrawScene::addDrawable(Drawable& drawable)
  {
    MT_ASSERT(std::find(_drawables.begin(), _drawables.end(), &drawable) == _drawables.end());
    _drawables.push_back(&drawable);
  }

  inline void SimpleDrawScene::removeDrawable(Drawable& drawable)
  {
    Drawables::iterator iDrawable = std::find(_drawables.begin(),
                                              _drawables.end(),
                                              &drawable);
    if(iDrawable != _drawables.end()) _drawables.erase(iDrawable);
  }
}