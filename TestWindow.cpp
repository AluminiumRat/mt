#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/DescriptorPool.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

static Ref<DataBuffer> createUniformBuffer(Device& device)
{
  glm::vec4 color(0.5f, 0.5f, 1.0f, 1.0f);
  Ref<DataBuffer> uniformBuffer(new DataBuffer( device,
                                                sizeof(color),
                                                DataBuffer::UNIFORM_BUFFER));
  device.graphicQueue()->uploadToBuffer(*uniformBuffer,
                                        0,
                                        sizeof(color),
                                        &color);
  return uniformBuffer;
}

static Ref<DataBuffer> createVertexBuffer(Device& device)
{
  struct Vertex
  {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec4 color;
  };
  Vertex vertices[3] ={{.position = {0.0f, -0.5f, 0.0f},
                        .color = {1, 0, 0, 1}},
                       {.position = {0.5f, 0.5f, 0.0f},
                        .color = {0, 1, 0, 1}},
                       {.position = {-0.5f, 0.5f, 0.0f},
                        .color = {0, 0, 1, 1}}};
  Ref<DataBuffer> vertexBuffer(new DataBuffer( device,
                                               sizeof(vertices),
                                               DataBuffer::STORAGE_BUFFER));
  device.graphicQueue()->uploadToBuffer(*vertexBuffer,
                                        0,
                                        sizeof(vertices),
                                        vertices);
  return vertexBuffer;
}

static Ref<DescriptorSet> createDescriptorSet(GraphicPipeline& pipeline)
{
  Ref<DescriptorPool> pool(new DescriptorPool(
                                          pipeline.device(),
                                          pipeline.layout().descriptorCounter(),
                                          1,
                                          DescriptorPool::STATIC_POOL));
  VkDescriptorSetLayoutBinding bindings[2];
  bindings[0] = VkDescriptorSetLayoutBinding{};
  bindings[0].binding = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[1] = VkDescriptorSetLayoutBinding{};
  bindings[1].binding = 2;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  ConstRef<DescriptorSetLayout> setLayout =
                    ConstRef(new DescriptorSetLayout( pipeline.device(),
                                                      bindings));
  Ref<DescriptorSet> set = pool->allocateSet(*setLayout);

  Ref<DataBuffer> vertexBuffer = createVertexBuffer(pipeline.device());
  set->attachStorageBuffer(*vertexBuffer, 1);

  Ref<DataBuffer> uniformBuffer = createUniformBuffer(pipeline.device());
  set->attachUniformBuffer(*uniformBuffer, 2);

  return set;
}

TestWindow::TestWindow(Device& device) :
  GLFWRenderWindow(device, "Vulkan window"),
  _pipeline(_createPipeline()),
  _descriptorSet(createDescriptorSet(*_pipeline))
{
}

static Ref<PipelineLayout> createPipelineLayout(Device& device)
{
  ConstRef<DescriptorSetLayout> sets[2];
  sets[0] = ConstRef(new DescriptorSetLayout(device, {}));

  VkDescriptorSetLayoutBinding bindings[2];
  bindings[0] = VkDescriptorSetLayoutBinding{};
  bindings[0].binding = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[1] = VkDescriptorSetLayoutBinding{};
  bindings[1].binding = 2;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  sets[1] = ConstRef(new DescriptorSetLayout(device, bindings));

  return Ref(new PipelineLayout(device, sets));
}

Ref<GraphicPipeline> TestWindow::_createPipeline()
{
  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);

  ShaderModule vertShader(device(), "shader.vert.spv");
  ShaderModule fragShader(device(), "shader.frag.spv");
  AbstractPipeline::ShaderInfo shadersInfo[2]
    { { .module = &vertShader,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .entryPoint = "main"},
      { .module = &fragShader,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .entryPoint = "main"}};

  VkPipelineRasterizationStateCreateInfo rasterizationState{};
  rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.lineWidth = 1;

  VkPipelineDepthStencilStateCreateInfo depthStencilState{};
  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  VkPipelineColorBlendStateCreateInfo blendingState{};
  blendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendingState.attachmentCount = 1;
  VkPipelineColorBlendAttachmentState attachmentState{};
  attachmentState.colorWriteMask =  VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;
  blendingState.pAttachments = &attachmentState;

  Ref<PipelineLayout> pipelineLayout = createPipelineLayout(device());

  return Ref(new GraphicPipeline( fbFormat,
                                  shadersInfo,
                                  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                  rasterizationState,
                                  depthStencilState,
                                  blendingState,
                                  *pipelineLayout));
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  commandProducer.bindDescriptorSetGraphic( *_descriptorSet,
                                            1,
                                            _pipeline->layout());

  commandProducer.setGraphicPipeline(*_pipeline);
  commandProducer.draw(3);

  renderPass.endPass();
}