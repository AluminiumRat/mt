#include <imgui.h>

#include <hld/drawScene/Drawable.h>
#include <hld/DrawPlan.h>
#include <hld/FrameBuildContext.h>
#include <hld/HLDLib.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Pipeline/DescriptorPool.h>

#include <TestDrawStage.h>

using namespace mt;

TestDrawStage::TestDrawStage(Device& device) :
  _stageIndex(HLDLib::instance().getStageIndex(stageName)),
  _commandMemoryPool(4 * 1024),
  _drawCommands(_commandMemoryPool),
  _lastFrameCommandsCount(0)
{
  _createCommonSet(device);
}

void TestDrawStage::_createCommonSet(Device& device)
{
  VkDescriptorSetLayoutBinding commonSetBindings[1];
  commonSetBindings[0] = {};
  commonSetBindings[0].binding = 0;
  commonSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  commonSetBindings[0].descriptorCount = 1;
  commonSetBindings[0].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS |
                                    VK_SHADER_STAGE_COMPUTE_BIT;
  ConstRef commonSetLayout(new DescriptorSetLayout(device, commonSetBindings));

  Ref<DescriptorPool> pool(new DescriptorPool(
                                          device,
                                          commonSetLayout->descriptorCounter(),
                                          1,
                                          DescriptorPool::STATIC_POOL));
  _commonDescriptorSet = pool->allocateSet(*commonSetLayout);

  _commonUniformBuffer = new DataBuffer(device,
                                        sizeof(Camera::ShaderData),
                                        DataBuffer::UNIFORM_BUFFER,
                                        "CameraData");

  _commonDescriptorSet->attachUniformBuffer(*_commonUniformBuffer, 0);
  _commonDescriptorSet->finalize();

  _pipelineLayout = new PipelineLayout( device,
                                        std::span(&commonSetLayout, 1));
}

void TestDrawStage::draw( CommandProducerGraphic& commandProducer,
                          const DrawPlan& drawPlan,
                          const FrameBuildContext& frameContext)
{
  _updateCommonSet(commandProducer, *frameContext.viewCamera);
  commandProducer.bindDescriptorSetGraphic( *_commonDescriptorSet,
                                            (uint32_t)DescriptorSetType::COMMON,
                                            *_pipelineLayout);
  _processDrawables(commandProducer, drawPlan, frameContext);

  commandProducer.unbindDescriptorSetGraphic(
                                          (uint32_t)DescriptorSetType::COMMON);
}

void TestDrawStage::_updateCommonSet( CommandProducerGraphic& commandProducer,
                                      const Camera& camera)
{
  Camera::ShaderData cameraData = camera.makeShaderData();
  UniformMemoryPool::MemoryInfo uploadedData =
                      commandProducer.uniformMemorySession().write(cameraData);
  commandProducer.uniformBufferTransfer(*uploadedData.buffer,
                                        *_commonUniformBuffer,
                                        uploadedData.offset,
                                        0,
                                        sizeof(Camera::ShaderData));
}

void TestDrawStage::_processDrawables(CommandProducerGraphic& commandProducer,
                                      const DrawPlan& drawPlan,
                                      const FrameBuildContext& frameContext)
{
  _drawCommands.clear();
  _commandMemoryPool.reset();

  _drawCommands.fillFromStagePlan(drawPlan.stagePlan(_stageIndex),
                                  frameContext,
                                  _stageIndex,
                                  nullptr);

  _lastFrameCommandsCount = _drawCommands.size();

  _drawCommands.draw( commandProducer,
                      DrawCommandList::BY_GROUP_INDEX_SORTING);
}

void TestDrawStage::makeImGui() const
{
  ImGui::Text("Commands: %d", (int)_lastFrameCommandsCount);
}
