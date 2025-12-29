#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <gui/ImGuiRAII.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <TextureViewer.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/Device.h>

TextureViewer::TextureViewer() :
  _descriptorSet(VK_NULL_HANDLE)
{
}

TextureViewer::~TextureViewer() noexcept
{
  _clearResources();
}

void TextureViewer::makeGUI(const char* id, const mt::Image& image)
{
  if(&image != _lastImage.get() && _lastImage != nullptr) _clearResources();

  if(_lastImage == nullptr)
  {
    _lastImage = mt::ConstRef(&image);

    _lastImageView = mt::Ref(new mt::ImageView( *_lastImage,
                                                mt::ImageSlice(*_lastImage),
                                                VK_IMAGE_VIEW_TYPE_2D));

    mt::Device& device = _lastImage->device();
    _sampler = mt::Ref(new mt::Sampler(device));

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
    _descriptorSet = pool->allocateSet(*setLayout);
    _descriptorSet->attachCombinedImageSampler( *_lastImageView,
                                                *_sampler,
                                                0,
                                                VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  ImGui::ImageWithBg( (ImTextureID)_descriptorSet->handle(),
                      ImVec2( (float)_lastImage->extent().x,
                              (float)_lastImage->extent().y),
                      ImVec2(0.0f, 0.0f),
                      ImVec2(1.0f, 1.0f),
                      ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void TextureViewer::_clearResources() noexcept
{
  if(_lastImage != nullptr)
  {
    mt::Device& device = _lastImage->device();
    MT_ASSERT(device.graphicQueue() != nullptr);

    try
    {
      device.graphicQueue()->waitIdle();
    }
    catch(std::exception& error)
    {
      mt::Log::error() << "TextureViewer::_clearResources: " << error.what();
      mt::Abort(error.what());
    }
  }

  _lastImage.reset();
  _lastImageView.reset();
  _sampler.reset();
  _descriptorSet.reset();
}
