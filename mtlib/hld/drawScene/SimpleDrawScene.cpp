#include <hld/drawScene/SimpleDrawScene.h>
#include <hld/drawScene/Drawable.h>

using namespace mt;

void SimpleDrawScene::fillDrawPlan( DrawPlan& plan,
                                    Camera& camera,
                                    uint32_t frameTypeIndex) const
{
  for(Drawable* drawable : _drawables)
  {
    drawable->addToDrawPlan(plan, frameTypeIndex);
  }
}