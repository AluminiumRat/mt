#include <hld/drawScene/DrawScene.h>
#include <hld/drawScene/Drawable.h>
#include <hld/FrameBuildContext.h>
#include <util/Camera.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/queue/CommandQueueCompute.h>

using namespace mt;

DrawScene::DrawScene() noexcept :
  _needUpdateTLAS(false),
  _needRebuildTLAS(false)
{
}

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

void DrawScene::updateGPUData(CommandProducerCompute& commandProducer)
{
  if(_needUpdateTLAS || _needRebuildTLAS)
  {
    BLASInstances instances;
    instances.reserve(_blasList.size());
    for(BLASInstance* instance : _blasList) instances.push_back(*instance);

    if(_tlas == nullptr || _needRebuildTLAS)
    {
      _tlas.reset();
      if(!instances.empty())
      {
        _tlas = new TLAS( commandProducer.queue().device(),
                          instances,
                          "SceneTLAS");
        _tlas->build(commandProducer);
      }
    }
    else _tlas->update(instances, commandProducer);

    _needUpdateTLAS = false;
    _needRebuildTLAS = false;
  }
}
