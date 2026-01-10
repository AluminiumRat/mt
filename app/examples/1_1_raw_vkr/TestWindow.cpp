#include <vkr/image/Image.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/Device.h>
#include <vkr/Sampler.h>
#include <TestWindow.h>

using namespace mt;

static Ref<DescriptorSet> createDescriptorSet(GraphicPipeline& pipeline);

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Vulkan window"),
  _pipeline(_createPipeline()),
  _descriptorSet(_createDescriptorSet(*_pipeline))
{
}

Ref<GraphicPipeline> TestWindow::_createPipeline()
{
  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);

  ShaderModule vertShader(device(), "examples/raw_vkr/shader.vert.spv");
  ShaderModule fragShader(device(), "examples/raw_vkr/shader.frag.spv");
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

  Ref<PipelineLayout> pipelineLayout = _createPipelineLayout();

  return Ref(new GraphicPipeline( fbFormat,
                                  shadersInfo,
                                  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                  rasterizationState,
                                  depthStencilState,
                                  blendingState,
                                  *pipelineLayout));
}

Ref<PipelineLayout> TestWindow::_createPipelineLayout()
{
  ConstRef<DescriptorSetLayout> sets[1];
  sets[0] = _createSetLayout();
  return Ref(new PipelineLayout(device(), sets));
}

ConstRef <DescriptorSetLayout> TestWindow::_createSetLayout()
{
  VkDescriptorSetLayoutBinding bindings[4];
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

  bindings[2] = VkDescriptorSetLayoutBinding{};
  bindings[2].binding = 3;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[3] = VkDescriptorSetLayoutBinding{};
  bindings[3].binding = 4;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  return ConstRef(new DescriptorSetLayout(device(), bindings));
}

Ref<DescriptorSet> TestWindow::_createDescriptorSet(GraphicPipeline& pipeline)
{
  Ref<DescriptorPool> pool(new DescriptorPool(
                                          device(),
                                          pipeline.layout().descriptorCounter(),
                                          1,
                                          DescriptorPool::STATIC_POOL));
  ConstRef<DescriptorSetLayout> setLayout = _createSetLayout();
  Ref<DescriptorSet> set = pool->allocateSet(*setLayout);

  Ref<DataBuffer> vertexBuffer = _createVertexBuffer();
  set->attachStorageBuffer(*vertexBuffer, 1);

  Ref<DataBuffer> uniformBuffer = _createUniformBuffer();
  set->attachUniformBuffer(*uniformBuffer, 2);

  Ref<ImageView> texture = _createTexture();
  set->attachSampledImage(*texture, 3, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  Ref<Sampler> sampler(new Sampler(pipeline.device()));
  set->attachSampler(*sampler, 4);

  set->finalize();

  return set;
}

Ref<ImageView> TestWindow::_createTexture()
{
  Ref<Image> image(new Image( device(),
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              0,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              glm::uvec3(2, 2, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              true,
                              "Test image"));

  uint32_t pixels[4] = { 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000 };
  device().graphicQueue()->uploadToImage( *image,
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          0,
                                          0,
                                          glm::uvec3(0,0,0),
                                          glm::uvec3(2,2,1),
                                          pixels);

  return Ref(new ImageView( *image,
                            ImageSlice(*image),
                            VK_IMAGE_VIEW_TYPE_2D));
}

Ref<DataBuffer> TestWindow::_createUniformBuffer()
{
  glm::vec4 color(0.5f, 0.5f, 1.0f, 1.0f);
  Ref<DataBuffer> uniformBuffer(new DataBuffer( device(),
                                                sizeof(color),
                                                DataBuffer::UNIFORM_BUFFER,
                                                "Uniform buffer"));
  device().graphicQueue()->uploadToBuffer(*uniformBuffer,
                                          0,
                                          sizeof(color),
                                          &color);
  return uniformBuffer;
}

Ref<DataBuffer> TestWindow::_createVertexBuffer()
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
  Ref<DataBuffer> vertexBuffer(new DataBuffer( device(),
                                               sizeof(vertices),
                                               DataBuffer::STORAGE_BUFFER,
                                               "Vertex buffer"));
  device().graphicQueue()->uploadToBuffer(*vertexBuffer,
                                          0,
                                          sizeof(vertices),
                                          vertices);
  return vertexBuffer;
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  commandProducer.bindDescriptorSetGraphic( *_descriptorSet,
                                            0,
                                            _pipeline->layout());

  commandProducer.setGraphicPipeline(*_pipeline);
  commandProducer.draw(3);

  commandProducer.unbindDescriptorSetGraphic(1);

  renderPass.endPass();
}