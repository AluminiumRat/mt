#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <gui/TextureViewer/TextureViewer.h>
#include <gui/ImGuiRAII.h>
#include <gui/ImGuiWidgets.h>
#include <technique/TechniqueLoader.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/Device.h>

using namespace mt;

TextureViewer::TextureViewer(Device& device) :
  _device(device),
  _flatPass(nullptr),
  _cubemapPass(nullptr),
  _invCubemapPass(nullptr),
  _flatTexture(nullptr),
  _cubemapTexture(nullptr),
  _viewProjectionMatrix(nullptr),
  _modelMatrix(nullptr),
  _brightnessUniform(nullptr),
  _mipUniform(nullptr),
  _layerUniform(nullptr),
  _samplerSelection(nullptr),
  _cubemapAllowed(false),
  _viewType(FLAT_VIEW),
  _samplerType(NEAREST_SAMPLER),
  _brightness(1.0f),
  _mipIndex(0),
  _layerIndex(0),
  _imGuiSampler(new Sampler(device))
{
  MT_ASSERT(_device.graphicQueue() != nullptr);

  Ref techniqueConfigurator(
                          new TechniqueConfigurator(device, "View technique"));
  loadConfigurator(*techniqueConfigurator, "textureViewer/viewer.tch");
  techniqueConfigurator->rebuildConfiguration();
  _viewTechnique = Ref(new Technique(*techniqueConfigurator));
  _flatPass = &_viewTechnique->getOrCreatePass("FlatPass");
  _cubemapPass = &_viewTechnique->getOrCreatePass("CubemapPass");
  _invCubemapPass = &_viewTechnique->getOrCreatePass("InvCubemapPass");
  _flatTexture = &_viewTechnique->getOrCreateResourceBinding("flatTexture");
  _cubemapTexture =
                  &_viewTechnique->getOrCreateResourceBinding("cubemapTexture");
  _viewProjectionMatrix =
            &_viewTechnique->getOrCreateUniform("renderParams.viewProjMatrix");
  _modelMatrix =
            &_viewTechnique->getOrCreateUniform("renderParams.modelMatrix");
  _brightnessUniform =
                &_viewTechnique->getOrCreateUniform("renderParams.brightness");
  _mipUniform = &_viewTechnique->getOrCreateUniform("renderParams.mipIndex");
  _layerUniform = &_viewTechnique->getOrCreateUniform("renderParams.layer");
  _samplerSelection = &_viewTechnique->getOrCreateSelection("NEAREST_SAMPLER");

  _flatManipulator.setCamera(&_viewCamera);
  _orbitalManipulator.setCamera(&_viewCamera);

  //  Создаем объекты, необходимые для отрисовки текстуры в ImGui
  DescriptorCounter counters{};
  counters.combinedImageSamplers = 1;

  Ref pool(new DescriptorPool(device,
                              counters,
                              1,
                              DescriptorPool::STATIC_POOL));
  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  ConstRef setLayout(new DescriptorSetLayout( device,
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
    Log::error() << "TextureViewer::_clearResources: " << error.what();
    Abort(error.what());
  }

  _renderTargetImage.reset();
  _frameBuffer.reset();
}

glm::uvec2 TextureViewer::_getWidgetSize(const ImVec2& userSize) const
{
  glm::uvec2 widgetSize(0);

  if(userSize.x >= 0) widgetSize.x = (uint32_t)userSize.x;
  else
  {
    widgetSize.x = (uint32_t)std::max( ImGui::GetContentRegionAvail().x, 0.0f);
  }

  if (userSize.y >= 0) widgetSize.y = (uint32_t)userSize.y;
  else
  {
    widgetSize.y = (uint32_t)std::max(ImGui::GetContentRegionAvail().y, 0.0f);
  }

  return widgetSize;
}

void TextureViewer::_adjustFlatView(glm::uvec2 widgetSize)
{
  float textureAspectRatio = (float)_renderedImage->extent().x /
                                                    _renderedImage->extent().y;
  // Размер прямоугольника, на который мы натягиваем текстуру
  glm::vec2 imageSizeWorld(textureAspectRatio, 1.0f);

  float widgetAspectRatio = (float)widgetSize.x / widgetSize.y;

  glm::vec2 frustumOrigin(-0.1f, -0.1f);
  glm::vec2 frustumSize(1.2f, 1.2f);

  if(textureAspectRatio > widgetAspectRatio)
  {
    //  текстура более вытянута по горизонтали чем виджет
    frustumSize.x = 1.2f * imageSizeWorld.x;
    frustumSize.y = frustumSize.x / widgetAspectRatio;
    frustumOrigin.x = -0.1f * imageSizeWorld.x;
    frustumOrigin.y = -0.5f * (frustumSize.y - 1.0f);
  }
  else
  {
    //  виджет более вытянут по горизонтали чем текстура
    frustumSize.x = 1.2f * widgetAspectRatio;
    frustumOrigin.x = -0.5f * (frustumSize.x - textureAspectRatio);
  }

  _flatManipulator.setFrustumOrigin(frustumOrigin);
  _flatManipulator.setFrustumSize(frustumSize);
}

void TextureViewer::makeGUI(const char* id, const Image& image, ImVec2 size)
{
  ImGuiPushID pushID(id);

  //  Если изменился размер текстуры, то необходимо будет перенастроить
  //  камеру
  bool needAdjustFlatView = _renderedImage == nullptr ||
                            _renderedImage->extent().x != image.extent().x ||
                            _renderedImage->extent().y != image.extent().y;

  if(_renderedImage.get() != &image) _setNewRenderedImage(image);

  _makeControlWidgets();

  glm::uvec2 widgetSize = _getWidgetSize(size);
  if(widgetSize.x == 0 || widgetSize.y == 0) return;

  if(needAdjustFlatView) _adjustFlatView(widgetSize);

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

void TextureViewer::_viewTypeCombo()
{
  if(_cubemapAllowed)
  {
    static const Bimap<ViewType> viewTypeMap{ "View types",
                                              {
                                                {FLAT_VIEW, "Flat"},
                                                {CUBEMAP_VIEW, "Cubemap"},
                                                {INV_CUBEMAP_VIEW, "InvCubemap"}
                                              }};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
    enumSelectionCombo("##view type", _viewType, viewTypeMap);
  }
  else
  {
    _viewType = FLAT_VIEW;
    static const Bimap<ViewType> viewTypeMap{ "View types",
                                                  {{FLAT_VIEW, "Flat"}}};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
    enumSelectionCombo("##view type", _viewType, viewTypeMap);
  }
}

void TextureViewer::_makeControlWidgets()
{
  MT_ASSERT(_renderedImage != nullptr);

  _viewTypeCombo();

  //  Кобо бокс для выбора сэмплера
  sameLineIfPossible(ImGui::GetFontSize() * 13);
  ImGui::Text("Sampler:");
  ImGui::SameLine();
  static const Bimap<SamplerType> samplerMap{
    "Sampoler types",
    {
      {NEAREST_SAMPLER, "NEAREST"},
      {LINEAR_SAMPLER, "LINEAR"}
    }};
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
  enumSelectionCombo("##samplerCombo", _samplerType, samplerMap);

  sameLineIfPossible(ImGui::GetFontSize() * 15);
  ImGui::Text("Brightness:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
  ImGui::InputFloat("##brightness", &_brightness, 0.1f, 1.0f, "%.2f");

  sameLineIfPossible(ImGui::GetFontSize() * 10);
  ImGui::Text("Mip:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
  ImGui::InputInt("##mip", &_mipIndex);
  _mipIndex = glm::clamp( _mipIndex,
                          0,
                          (int)_renderedImage->mipmapCount() - 1);

  _layerInput();
}

void TextureViewer::_layerInput()
{
  sameLineIfPossible(ImGui::GetFontSize() * 12);
  ImGui::Text("Layer:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
  ImGui::InputInt("##layer", &_layerIndex);

  int maxLayerIndex = _viewType == FLAT_VIEW ?
                                    (int)_renderedImage->arraySize() - 1 :
                                    (int)_renderedImage->arraySize() / 6 - 1;
  _layerIndex = glm::clamp( _layerIndex, 0, maxLayerIndex);
}

void TextureViewer::_makeUndraggedArea(glm::uvec2 widgetSize)
{
  ImVec2 leftTopCorner = ImGui::GetCursorScreenPos();

  ImGui::ColorButton( "##undraggedArea",
                      ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                      ImGuiColorEditFlags_NoTooltip |
                        ImGuiColorEditFlags_NoDragDrop,
                      ImVec2((float)widgetSize.x, (float)widgetSize.y));

  ImGui::SetCursorScreenPos(leftTopCorner);
}

void TextureViewer::_rebuildRenderTarget(glm::uvec2 widgetSize)
{
  _clearRenderTarget();

  Ref newRenderTarget(new Image(_device,
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
  Ref imageView(new ImageView(*newRenderTarget,
                              ImageSlice(*newRenderTarget),
                              VK_IMAGE_VIEW_TYPE_2D));

  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = imageView.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.1f, 0.1f, 0.1f, 1.0f} };
  _frameBuffer = Ref(new FrameBuffer( std::span(&colorAttachment, 1),
                                      nullptr));

  _imGuiDescriptorSet->attachCombinedImageSampler(*imageView,
                                                  *_imGuiSampler,
                                                  0,
                                                  VK_SHADER_STAGE_FRAGMENT_BIT);

  _renderTargetImage = newRenderTarget;
}

void TextureViewer::_setNewRenderedImage(const Image& image)
{
  try
  {
    _renderedImage = ConstRef(&image);
    _flatTexture->setImage(new ImageView( image,
                                          ImageSlice(image),
                                          VK_IMAGE_VIEW_TYPE_2D_ARRAY));
    //  Для того чтобы из текстуры можно было сделать кубемапу, она должна быть
    //  квадратной и в ней должно быть количество слоев, кратное 6
    _cubemapAllowed =
                  _renderedImage->extent().x == _renderedImage->extent().y &&
                  _renderedImage->arraySize() % 6 == 0;
    if(_cubemapAllowed)
    {
      _cubemapTexture->setImage(new ImageView(image,
                                              ImageSlice(image),
                                              VK_IMAGE_VIEW_TYPE_CUBE_ARRAY));
    }
    else
    {
      _cubemapTexture->clear();
    }
  }
  catch(...)
  {
    _flatTexture->clear();
    _cubemapTexture->clear();
    _renderedImage.reset();
    _cubemapAllowed = false;
    throw;
  }
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

  CommandQueueGraphic* queue = _device.graphicQueue();
  MT_ASSERT(queue != nullptr)
  std::unique_ptr<CommandProducerGraphic> producer = queue->startCommands();

  producer->imageBarrier( *_renderTargetImage,
                          ImageSlice(*_renderTargetImage),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          0,
                          0,
                          0,
                          0);

  CommandProducerGraphic::RenderPass renderPass(*producer, *_frameBuffer);

  _drawGeometry(*producer);

  renderPass.endPass();

  producer->imageBarrier( *_renderTargetImage,
                          ImageSlice(*_renderTargetImage),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

  queue->submitCommands(std::move(producer));
}

void TextureViewer::_drawGeometry(CommandProducerGraphic& producer)
{
  switch(_viewType)
  {
  case FLAT_VIEW:
    {
      Technique::Bind bind(*_viewTechnique, *_flatPass, producer);
      if (bind.isValid())
      {
        producer.draw(4);
        bind.release();
      }
      break;
    }
  case CUBEMAP_VIEW:
    {
      Technique::Bind bind(*_viewTechnique, *_cubemapPass, producer);
      if (bind.isValid())
      {
        producer.draw(36);
        bind.release();
      }
      break;
  }
  case INV_CUBEMAP_VIEW:
    {
      Technique::Bind bind(*_viewTechnique, *_invCubemapPass, producer);
      if (bind.isValid())
      {
        producer.draw(36);
        bind.release();
      }
      break;
    }
  }
}
