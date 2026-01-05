#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <gui/ImGuiRAII.h>
#include <gui/ImGuiWidgets.h>
#include <technique/TechniqueLoader.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/Device.h>

#include <TextureViewer.h>

TextureViewer::TextureViewer(mt::Device& device) :
  _device(device),
  _texture(nullptr),
  _flatPass(nullptr),
  _viewProjectionMatrix(nullptr),
  _modelMatrix(nullptr),
  _brightnessUniform(nullptr),
  _mipUniform(nullptr),
  _layerUniform(nullptr),
  _samplerSelection(nullptr),
  _flatManipulator(_viewCamera),
  _orbitalManipulator(_viewCamera),
  _viewType(FLAT_VIEW),
  _samplerType(NEAREST_SAMPLER),
  _brightness(1.0f),
  _mipIndex(0),
  _layerIndex(0),
  _imGuiSampler(new mt::Sampler(device))
{
  MT_ASSERT(_device.graphicQueue() != nullptr);

  mt::Ref squareConfigurator(new mt::TechniqueConfigurator(
                                                          device,
                                                          "View technique"));
  mt::loadConfigurator(*squareConfigurator, "textureViewer/viewer.tch");
  squareConfigurator->rebuildConfiguration();
  _viewTechnique = mt::Ref(new mt::Technique(*squareConfigurator));
  _texture = &_viewTechnique->getOrCreateResourceBinding("colorTexture");
  _flatPass = &_viewTechnique->getOrCreatePass("FlatPass");
  _viewProjectionMatrix =
            &_viewTechnique->getOrCreateUniform("renderParams.viewProjMatrix");
  _modelMatrix =
            &_viewTechnique->getOrCreateUniform("renderParams.modelMatrix");
  _brightnessUniform =
                &_viewTechnique->getOrCreateUniform("renderParams.brightness");
  _mipUniform = &_viewTechnique->getOrCreateUniform("renderParams.mipIndex");
  _layerUniform = &_viewTechnique->getOrCreateUniform("renderParams.layer");
  _samplerSelection = &_viewTechnique->getOrCreateSelection("NEAREST_SAMPLER");

  _flatManipulator.setFrustumOrigin(glm::vec2(-0.1f, - 0.1f));
  _flatManipulator.setFrustumSize(glm::vec2(1.2f, 1.2f));

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
  mt::ImGuiPushID pushID(id);

  if (_renderedImage.get() != &image) _setNewRenderedImage(image);

  _makeControlWidgets();

  glm::uvec2 widgetSize = _getWidgetSize(size);
  if(widgetSize.x == 0 || widgetSize.y == 0) return;

  _makeUndraggedArea(widgetSize);

  if(_viewType == FLAT_VIEW)
  {
    _flatManipulator.update(ImGui::GetCursorScreenPos(),
                            ImVec2((float)widgetSize.x, (float)widgetSize.y));
  }
  else
  {
    _orbitalManipulator.update(
                              ImGui::GetCursorScreenPos(),
                              ImVec2((float)widgetSize.x, (float)widgetSize.y));
  }

  if( _renderTargetImage == nullptr ||
      widgetSize != glm::uvec2(_renderTargetImage->extent()))
  {
    _rebuildRenderTarget(widgetSize);
  }

  _renderScene();

  ImGui::ImageWithBg( (ImTextureID)_imGuiDescriptorSet->handle(),
                      ImVec2((float)widgetSize.x, (float)widgetSize.y),
                      ImVec2(0.0f, 0.0f),
                      ImVec2(1.0f, 1.0f),
                      ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void TextureViewer::_makeControlWidgets()
{
  MT_ASSERT(_renderedImage != nullptr);

  //  Комбо бокс для ViewType
  static const mt::Bimap<ViewType> viewTypeMap{
    "View types",
    {
      {FLAT_VIEW, "Flat"},
      {CUBEMAP_VIEW, "Cubemap"}
    }};
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
  mt::enumSelectionCombo("##view type", _viewType, viewTypeMap);

  //  Кобо бокс для выбора сэмплера
  mt::sameLineIfPossible(ImGui::GetFontSize() * 13);
  ImGui::Text("Sampler:");
  ImGui::SameLine();
  static const mt::Bimap<SamplerType> samplerMap{
    "Sampoler types",
    {
      {NEAREST_SAMPLER, "NEAREST"},
      {LINEAR_SAMPLER, "LINEAR"}
    }};
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
  mt::enumSelectionCombo("##samplerCombo", _samplerType, samplerMap);

  mt::sameLineIfPossible(ImGui::GetFontSize() * 15);
  ImGui::Text("Brightness:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
  ImGui::InputFloat("##brightness", &_brightness, 0.1f, 1.0f, "%.2f");

  mt::sameLineIfPossible(ImGui::GetFontSize() * 10);
  ImGui::Text("Mip:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
  ImGui::InputInt("##mip", &_mipIndex);
  _mipIndex = glm::clamp( _mipIndex,
                          0,
                          (int)_renderedImage->mipmapCount() - 1);

  mt::sameLineIfPossible(ImGui::GetFontSize() * 12);
  ImGui::Text("Layer:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
  ImGui::InputInt("##layer", &_layerIndex);
  _layerIndex = glm::clamp( _layerIndex,
                            0,
                            (int)_renderedImage->arraySize() - 1);
}

void TextureViewer::_makeUndraggedArea(glm::uvec2 widgetSize)
{
  ImVec2 leftTopCorner = ImGui::GetCursorScreenPos();
  ImGui::ColorButton( "##undraggedArea",
                      ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                      ImGuiColorEditFlags_NoTooltip |
                        ImGuiColorEditFlags_NoDragDrop,
                      ImVec2((float)widgetSize.x, (float)widgetSize.y));
  if(ImGui::IsItemHovered())
  {
    //  Имитируем поведение для камера-манипулятора, как-будто курсор находится
    //  вне окна
    ImGui::SetNextFrameWantCaptureMouse(false);
    ImGui::SetNextFrameWantCaptureKeyboard(false);
  }
  ImGui::SetCursorScreenPos(leftTopCorner);
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

void TextureViewer::_setNewRenderedImage(const mt::Image& image)
{
  mt::ConstRef<mt::ImageView> newImageView =
                  mt::ConstRef(new mt::ImageView( image,
                                                  mt::ImageSlice(image),
                                                  VK_IMAGE_VIEW_TYPE_2D_ARRAY));
  _texture->setImage(newImageView);
  _renderedImageView = newImageView;
  _renderedImage = mt::ConstRef(&image);
}

void TextureViewer::_updateRenderParams()
{
  _viewProjectionMatrix->setValue(_viewCamera.projectionMatrix() *
                                                      _viewCamera.viewMatrix());

  //  Для Flat техники выставляем модельную матрицу так, чтобы сохранить
  //  соотношение сторон
  glm::mat4 newModelMatrix(1);
  newModelMatrix[0][0] = (float)_renderedImage->extent().x /
                                                    _renderedImage->extent().y;
  _modelMatrix->setValue(newModelMatrix);

  if(_samplerType == NEAREST_SAMPLER) _samplerSelection->setValue("1");
  else _samplerSelection->setValue("0");

  _brightnessUniform->setValue(_brightness);

  _mipUniform->setValue((float)_mipIndex);
  _layerUniform->setValue((float)_layerIndex);
}

void TextureViewer::_renderScene()
{
  _updateRenderParams();

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

  mt::Technique::Bind bind( *_viewTechnique,
                            *_flatPass,
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
