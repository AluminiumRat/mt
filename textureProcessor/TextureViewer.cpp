#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <gui/ImGuiRAII.h>
#include <technique/TechniqueLoader.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/Device.h>

#include <TextureViewer.h>

TextureViewer::TextureViewer(mt::Device& device) :
  _device(device),
  _imGuiSampler(new mt::Sampler(device))
{
  MT_ASSERT(_device.graphicQueue() != nullptr);

  mt::Ref squareConfigurator(new mt::TechniqueConfigurator(
                                                          device,
                                                          "Square technique"));
  mt::loadConfigurator(*squareConfigurator, "textureViewer/square.tch");
  squareConfigurator->rebuildConfiguration();
  _squareTechnique = mt::Ref(new mt::Technique(*squareConfigurator));
  _squareProps.texture =
                  &_squareTechnique->getOrCreateResourceBinding("colorTexture");
  _squareProps.renderPass = &_squareTechnique->getOrCreatePass("RenderPass");

  //  Создаем объекты, необходимые для отрисовки текстуры в ImGui
  mt::DescriptorCounter counters{};
  counters.combinedImageSamplers = 1;

  mt::Ref pool(new mt::DescriptorPool(device,
                                      counters,
                                      1,
                                      mt::DescriptorPool::STATIC_POOL));
  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  mt::ConstRef setLayout(new mt::DescriptorSetLayout(
                                                    device,
                                                    std::span(&binding, 1)));
  _imGuiDescriptorSet = pool->allocateSet(*setLayout);
}

TextureViewer::~TextureViewer() noexcept
{
  _clearRenderTarget();
}

void TextureViewer::_clearRenderTarget() noexcept
{
  try
  {
    _device.graphicQueue()->waitIdle();
  }
  catch(std::exception& error)
  {
    mt::Log::error() << "TextureViewer::_clearResources: " << error.what();
    mt::Abort(error.what());
  }

  _renderTargetImage.reset();
  _frameBuffer.reset();
}

glm::uvec2 TextureViewer::_getWidgetSize(const ImVec2& userSize) const
{
  glm::uvec2 widgetSize(0);

  if(userSize.x >= 0) widgetSize.x = (uint32_t)userSize.x;
  else widgetSize.x = (uint32_t)ImGui::GetContentRegionAvail().x;

  if (userSize.y >= 0) widgetSize.y = (uint32_t)userSize.y;
  else widgetSize.y = (uint32_t)ImGui::GetContentRegionAvail().y;

  return widgetSize;
}

void TextureViewer::makeGUI(const char* id,
                            const mt::Image& image,
                            ImVec2 size)
{
  glm::uvec2 widgetSize = _getWidgetSize(size);
  if(widgetSize.x == 0 || widgetSize.y == 0) return;

  if( _renderTargetImage == nullptr ||
      widgetSize != glm::uvec2(_renderTargetImage->extent()))
  {
    _rebuildRenderTarget(widgetSize);
  }

  if(_renderedImage.get() != &image)
  {
    mt::ConstRef<mt::ImageView> newImageView =
                        mt::ConstRef(new mt::ImageView( image,
                                                        mt::ImageSlice(image),
                                                        VK_IMAGE_VIEW_TYPE_2D));
    _squareProps.texture->setImage(newImageView);
    _renderedImageView = newImageView;
    _renderedImage = mt::ConstRef(&image);
  }

  _renderScene();

  ImGui::ImageWithBg( (ImTextureID)_imGuiDescriptorSet->handle(),
                      ImVec2((float)widgetSize.x, (float)widgetSize.y),
                      ImVec2(0.0f, 0.0f),
                      ImVec2(1.0f, 1.0f),
                      ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void TextureViewer::_rebuildRenderTarget(glm::uvec2 widgetSize)
{
  _clearRenderTarget();

  mt::Ref newRenderTarget(new mt::Image(_device,
                                        VK_IMAGE_TYPE_2D,
                                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                        0,
                                        VK_FORMAT_B8G8R8A8_SRGB,
                                        glm::uvec3(widgetSize, 1),
                                        VK_SAMPLE_COUNT_1_BIT,
                                        1,
                                        1,
                                        false,
                                        "TextureViewerRenderTarget"));
  mt::Ref imageView(new mt::ImageView(*newRenderTarget,
                                      mt::ImageSlice(*newRenderTarget),
                                      VK_IMAGE_VIEW_TYPE_2D));

  mt::FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = imageView.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.1f, 0.1f, 0.1f, 1.0f} };
  _frameBuffer = mt::Ref(new mt::FrameBuffer( std::span(&colorAttachment, 1),
                                              nullptr));

  _imGuiDescriptorSet->attachCombinedImageSampler(*imageView,
                                                  *_imGuiSampler,
                                                  0,
                                                  VK_SHADER_STAGE_FRAGMENT_BIT);

  _renderTargetImage = newRenderTarget;
}

void TextureViewer::_renderScene()
{
  mt::CommandQueueGraphic* queue = _device.graphicQueue();
  MT_ASSERT(queue != nullptr)
  std::unique_ptr<mt::CommandProducerGraphic> producer = queue->startCommands();

  producer->imageBarrier( *_renderTargetImage,
                          mt::ImageSlice(*_renderTargetImage),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          0,
                          0,
                          0,
                          0);

  mt::CommandProducerGraphic::RenderPass renderPass(*producer, *_frameBuffer);

  mt::Technique::Bind bind( *_squareTechnique,
                            *_squareProps.renderPass,
                            *producer);
  if (bind.isValid())
  {
    producer->draw(4);
    bind.release();
  }

  renderPass.endPass();

  producer->imageBarrier( *_renderTargetImage,
                          mt::ImageSlice(*_renderTargetImage),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

  queue->submitCommands(std::move(producer));
}
