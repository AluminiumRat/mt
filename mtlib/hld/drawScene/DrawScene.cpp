#include <hld/drawScene/DrawScene.h>
#include <hld/drawScene/Drawable.h>
#include <hld/FrameBuildContext.h>
#include <util/Camera.h>

using namespace mt;

void DrawScene::fillDrawPlan( DrawPlan& plan,
                              const FrameBuildContext& frameContext) const
{
  for(Drawable* drawable : _drawables)
  {
    drawable->addToDrawPlan(plan, frameContext);
  }

  ViewFrustum viewFrustum = frameContext.viewCamera->worldFrustum();
  for (Drawable3D* drawable : _drawables3D)
  {
    if(viewFrustum.intersect(drawable->boundingBox()))
    {
      drawable->addToDrawPlan(plan, frameContext);
    }
  }
}