#pragma once

#include <gui/RenderWindow.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class TestWindow : public RenderWindow
  {
  public:
    TestWindow(Device& device);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    Ref<GraphicPipeline> _createPipeline();
    Ref<PipelineLayout> _createPipelineLayout();
    ConstRef <DescriptorSetLayout> _createSetLayout();
    Ref<DescriptorSet> _createDescriptorSet(GraphicPipeline& pipeline);
    Ref<ImageView> _createTexture();
    Ref<DataBuffer> _createUniformBuffer();
    Ref<DataBuffer> _createVertexBuffer();

  private:
    Ref<GraphicPipeline> _pipeline;
    Ref<DescriptorSet> _descriptorSet;
  };
}