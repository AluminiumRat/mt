#include <stdexcept>

#include <technique/Technique.h>
#include <util/Abort.h>
#include <util/Log.h>

using namespace mt;

Technique::Technique(Device& device) noexcept :
  _device(device),
  _revision(0),
  _topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
  _rasterizationState{},
  _depthStencilState{},
  _blendingState{}
{
  _rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  _rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  _rasterizationState.lineWidth = 1;

  _depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  for(VkPipelineColorBlendAttachmentState& attachment : _attachmentsBlending)
  {
    attachment = VkPipelineColorBlendAttachmentState{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
  }
  _blendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  _blendingState.attachmentCount = 1;
  _blendingState.pAttachments = _attachmentsBlending.data();
}

void Technique::_invalidateConfiguration() noexcept
{
  _configuration.reset();
  _revision++;
}

void Technique::_buildConfiguration()
{
  try
  {
    _configuration = Ref(new TechniqueConfiguration);
    _configuration->isValid = false;

    _configuration->topology = _topology;
    _configuration->rasterizationState = _rasterizationState;
    _configuration->depthStencilState = _depthStencilState;
    _configuration->blendingState = _blendingState;
    _configuration->blendingState.pAttachments =
                                      _configuration->attachmentsBlending.data();
    _configuration->attachmentsBlending = _attachmentsBlending;

    _configuration->isValid = true;
  }
  catch(std::exception& error)
  {
    //  Старого состояния не существует, а новое создать не получилось.
    //  Патовая ситуация.
    Log::error() << "Technique::_buildConfiguration: " << error.what();
    Abort("Unable to build technique configuration");
  }
}
