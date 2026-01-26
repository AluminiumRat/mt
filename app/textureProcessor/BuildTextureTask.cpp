#include <ddsSupport/ddsSupport.h>
#include <util/Assert.h>
#include <vkr/image/ImageView.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/queue/CommandQueueGraphic.h>

#include <Application.h>
#include <BuildTextureTask.h>

Project::BuildTextureTask::BuildTextureTask(
                                        const mt::Technique& technique,
                                        const std::filesystem::path& outputFile,
                                        VkFormat textureFormat,
                                        glm::uvec2 textureSize,
                                        uint32_t mipsCount,
                                        uint32_t arraySize,
                                        Project& project) :
  AsyncTask("Build texture",
            mt::AsyncTask::EXCLUSIVE_MODE,
            mt::AsyncTask::EXPLICIT),
  _outputFile(outputFile),
  _textureFormat(textureFormat),
  _textureSize(textureSize),
  _mipsCount(mipsCount),
  _arraySize(arraySize),
  _project(project)
{
  _textureSize = glm::max(_textureSize, 1u);
  _mipsCount = glm::clamp(_mipsCount,
                          1u,
                          mt::Image::calculateMipNumber(_textureSize));
  _arraySize = glm::max(_arraySize, 1u);
  _copyTechnique(technique);
}

void Project::BuildTextureTask::_copyTechnique(const mt::Technique& technique)
{
  //  Создаем копию переданной техники, чтобы избежать конфликтов
  //  в многопотоке

  const mt::TechniqueConfiguration* configuration = technique.configuration();
  MT_ASSERT(configuration != nullptr);

  // Создаем новую технику на той же конфигурации
  _technique.emplace( *technique.configuration(), "Build image technique");

  // Копируем в новую технику юниформы
  for(const mt::TechniqueConfiguration::UniformBuffer& buffer :
                                                  configuration->uniformBuffers)
  {
    if(buffer.set == mt::DescriptorSetType::COMMON) continue;
    for(const mt::TechniqueConfiguration::UniformVariable& variable :
                                                              buffer.variables)
    {
      const mt::UniformVariable* originUniform =
                                technique.getUniform(variable.fullName.c_str());
      if(originUniform == nullptr) continue;
      mt::UniformVariable& newUniform =
                      _technique->getOrCreateUniform(variable.fullName.c_str());
      newUniform.setValue(originUniform->getValue());
    }
  }

  // Копируем в новую технику ресурсы
  for (const mt::TechniqueConfiguration::Resource& resource :
                                                      configuration->resources)
  {
    if(resource.set == mt::DescriptorSetType::COMMON ||
        resource.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
      continue;
    }

    const mt::ResourceBinding* originBinding =
                            technique.getResourceBinding(resource.name.c_str());
    if(originBinding == nullptr || originBinding->empty()) continue;

    mt::ResourceBinding& newBinding =
                  _technique->getOrCreateResourceBinding(resource.name.c_str());
    if(originBinding->buffer() != nullptr)
    {
      newBinding.setBuffer(originBinding->buffer());
    }
    if(originBinding->image() != nullptr)
    {
      newBinding.setImage(originBinding->image());
    }
    if(originBinding->sampler() != nullptr)
    {
      newBinding.setSampler(originBinding->sampler());
    }
  }
}

void Project::BuildTextureTask::asyncPart()
{
  _createTargetImage();

  reportStage("Build texture");

  for(uint32_t arrayIndex = 0; arrayIndex < _arraySize; arrayIndex++)
  {
    for(uint32_t mipIndex = 0; mipIndex < _mipsCount; mipIndex++)
    {
      _buildSlice(mipIndex, arrayIndex);
      int percent = 100 * (mipIndex + arrayIndex * _mipsCount) /
                                                      (_arraySize * _mipsCount);
      reportPercent((uint8_t)percent);
      if(shouldBeAborted()) return;
    }
  }

  reportStage("Save texture");
  _saveTexture();
}

void Project::BuildTextureTask::_createTargetImage()
{
  mt::Device& device = Application::instance().primaryDevice();

  VkImageCreateFlags imageFlags = 0;
  if( _textureSize.x == _textureSize.y &&
      _arraySize % 6 == 0)
  {
    imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }
  _targetImage = mt::Ref<mt::Image>(new mt::Image(
                                            device,
                                            VK_IMAGE_TYPE_2D,
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT |
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            imageFlags,
                                            _textureFormat,
                                            glm::uvec3(_textureSize, 1),
                                            VK_SAMPLE_COUNT_1_BIT,
                                            _arraySize,
                                            _mipsCount,
                                            true,
                                            "Result texture"));
}

void Project::BuildTextureTask::_buildSlice(uint32_t mipIndex,
                                            uint32_t arrayIndex)
{
  _adjustIntrinsic(mipIndex, arrayIndex);

  mt::Ref<mt::FrameBuffer> frameBuffer = _createFrameBuffer(mipIndex,
                                                            arrayIndex);

  mt::Device& device = Application::instance().primaryDevice();
  mt::CommandQueueGraphic* queue = device.graphicQueue();
  MT_ASSERT(queue != nullptr)

  std::unique_ptr<mt::CommandProducerGraphic> producer = queue->startCommands();

  //  Рендерим полноэкранный квадрат в слайс
  mt::CommandProducerGraphic::RenderPass renderPass(*producer, *frameBuffer);
  mt::TechniquePass& techniquePass = _technique->getOrCreatePass("Pass");
  mt::Technique::Bind bind(*_technique, techniquePass, *producer);
  if (bind.isValid())
  {
    producer->draw(4);
    bind.release();
  }
  renderPass.endPass();

  queue->submitCommands(std::move(producer));
  queue->createSyncPoint().waitForReady();
}

void Project::BuildTextureTask::_adjustIntrinsic( uint32_t mipIndex,
                                                  uint32_t arrayIndex)
{
  mt::UniformVariable& mipUniform =
                          _technique->getOrCreateUniform("intrinsic.mipLevel");
  mipUniform.setValue(mipIndex);
  mt::UniformVariable& arrayIndexUniform =
                        _technique->getOrCreateUniform("intrinsic.arrayIndex");
  arrayIndexUniform.setValue(arrayIndex);
}

mt::Ref<mt::FrameBuffer> Project::BuildTextureTask::_createFrameBuffer(
                                                            uint32_t mipIndex,
                                                            uint32_t arrayIndex)
{
  mt::Ref<mt::ImageView> colorTarget(new mt::ImageView(
                                                *_targetImage,
                                                mt::ImageSlice(
                                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                                      mipIndex,
                                                      1,
                                                      arrayIndex,
                                                      1),
                                                VK_IMAGE_VIEW_TYPE_2D));
  mt::FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = colorTarget.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.0f, 0.0f, 0.00f, 0.0f}};

  return mt::Ref(new mt::FrameBuffer(std::span(&colorAttachment, 1), nullptr));
}

void Project::BuildTextureTask::_saveTexture()
{
  if(_outputFile.empty()) return;

  mt::Device& device = Application::instance().primaryDevice();
  mt::CommandQueueGraphic* queue = device.graphicQueue();
  MT_ASSERT(queue != nullptr)
  mt::saveToDDS(*_targetImage, _outputFile, queue);
}

void Project::BuildTextureTask::finalizePart()
{
  MT_ASSERT(_targetImage != nullptr);
  _project.setResultImage(*_targetImage);
}
