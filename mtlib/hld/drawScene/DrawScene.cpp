#include <hld/drawScene/DrawScene.h>
#include <hld/drawScene/Drawable.h>
#include <util/Camera.h>

using namespace mt;

void DrawScene::fillDrawPlan( DrawPlan& plan,
                                    Camera& camera,
                                    FrameTypeIndex frameTypeIndex) const
{
  for(Drawable* drawable : _drawables)
  {
    drawable->addToDrawPlan(plan, frameTypeIndex);
  }

  ViewFrustum viewFrustum = camera.worldFrustum();
  for (Drawable3D* drawable : _drawables3D)
  {
    if(viewFrustum.intersect(drawable->boundingBox()))
    {
      drawable->addToDrawPlan(plan, frameTypeIndex);
    }
  }
}