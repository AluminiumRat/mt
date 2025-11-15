#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <util/RefCounter.h>
#include <util/Ref.h>
#include <technique/TechniqueConfiguration.h>
#include <technique/DescriptorSetType.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/FrameBufferFormat.h>

struct SpvReflectBlockVariable;
struct SpvReflectDescriptorBinding;
struct SpvReflectDescriptorSet;
struct SpvReflectShaderModule;

namespace mt
{
  class Device;

  class Technique : public RefCounter
  {
  public:
    struct ShaderInfo
    {
      VkShaderStageFlagBits stage;
      std::string filename;
    };

  public:
    Technique(Device& device) noexcept;
    Technique(const Technique&) = delete;
    Technique& operator = (const Technique&) = delete;
  protected:
    ~Technique() noexcept = default;
  public:

    inline Device& device() const noexcept;

    //  Ревизия используется для отслеживания изменений в технике
    //  Каждый раз, когда происходтит изменение состояния, ревизия увеличивается
    //  и зависимым объектам необходимо синхронизироваться с техникой
    inline size_t revision() const noexcept;

    //  Может возвращать nullptr, если сборка конфигурации не завершилась
    //  успешно (например, ошибка в шейдере или неполный набор шейдеров)
    inline const TechniqueConfiguration* configuration();

    //  Если конфигурация ещё не инициализированa, то сделать это прямо сейчас
    //  Позволяет избежать задержек при первом использовании техники из-за
    //    ленивой инициализации.
    inline void forceUpdate();

    inline AbstractPipeline::Type pipelineType() const noexcept;
    inline void setPipelineType(AbstractPipeline::Type newValue) noexcept;

    inline const std::vector<ShaderInfo>& shaders() const noexcept;
    inline void setShaders(std::span<const ShaderInfo> newShaders);

    //  Может возвращать nullptr, если формат не установлен
    inline const FrameBufferFormat* frameBufferFormat() const noexcept;
    //  Можно передавать nullptr или не устанавливать формат для compue техник
    inline const void setFrameBufferFormat(
                                  const FrameBufferFormat* newFormat) noexcept;

    inline VkPrimitiveTopology topology() const noexcept;
    inline void setTopology(VkPrimitiveTopology newValue) noexcept;

    //--------------------------
    inline float lineWidth() const noexcept;
    inline void setLineWidth(float newValue) noexcept;

    //--------------------------
    //  Depth конфигурация
    inline bool depthClampEnable() const noexcept;
    inline void setDepthClampEnable(bool newValue) noexcept;

    inline bool rasterizationDiscardEnable() const noexcept;
    inline void setRasterizationDiscardEnable(bool newValue) noexcept;

    inline VkPolygonMode polygonMode() const noexcept;
    inline void setPolygonMode(VkPolygonMode newValue) noexcept;

    inline VkCullModeFlags cullMode() const noexcept;
    inline void setCullMode(VkCullModeFlags newValue) noexcept;

    inline VkFrontFace frontFace() const noexcept;
    inline void setFrontFace(VkFrontFace newValue) noexcept;

    inline bool depthBiasEnable() const noexcept;
    inline void setDepthBiasEnable(bool newValue) noexcept;

    inline float depthBiasConstantFactor() const noexcept;
    inline void setDepthBiasConstantFactor(float newValue) noexcept;

    inline float depthBiasClamp() const noexcept;
    inline void setDepthBiasClamp(float newValue) noexcept;

    inline float depthBiasSlopeFactor() const noexcept;
    inline void setDepthBiasSlopeFactor(float newValue) noexcept;

    inline bool depthTestEnable() const noexcept;
    inline void setDepthTestEnable(bool newValue) noexcept;

    inline bool depthWriteEnable() const noexcept;
    inline void setDepthWriteEnable(bool newValue) noexcept;

    inline VkCompareOp depthCompareOp() const noexcept;
    inline void setDepthCompareOp(VkCompareOp newValue) noexcept;

    inline bool depthBoundsTestEnable() const noexcept;
    inline void setDepthBoundsTestEnable(bool newValue) noexcept;

    inline float minDepthBounds() const noexcept;
    inline void setMinDepthBounds(float newValue) noexcept;

    inline float maxDepthBounds() const noexcept;
    inline void setMaxDepthBounds(float newValue) noexcept;

    //--------------------------
    //  Stencil конфигурация
    inline VkStencilOpState frontStencilOp() const noexcept;
    inline void setFrontStencilOp(VkStencilOpState newValue) noexcept;

    inline VkStencilOpState backStencilOp() const noexcept;
    inline void setBackStencilOp(VkStencilOpState newValue) noexcept;

    //--------------------------
    //  Блэндинг конфигурация
    inline bool blendEnable(uint32_t attachmentIndex) const noexcept;
    inline void setBlendEnable( uint32_t attachmentIndex,
                                bool newValue) noexcept;

    inline VkBlendFactor srcColorBlendFactor(
                                      uint32_t attachmentIndex) const noexcept;
    inline void setSrcColorBlendFactor( uint32_t attachmentIndex,
                                        VkBlendFactor newValue) noexcept;

    inline VkBlendFactor dstColorBlendFactor(
                                      uint32_t attachmentIndex) const noexcept;
    inline void setDstColorBlendFactor( uint32_t attachmentIndex,
                                        VkBlendFactor newValue) noexcept;

    inline VkBlendOp colorBlendOp(uint32_t attachmentIndex) const noexcept;
    inline void setColorBlendOp(uint32_t attachmentIndex,
                                VkBlendOp newValue) noexcept;

    inline VkBlendFactor srcAlphaBlendFactor(
                                      uint32_t attachmentIndex) const noexcept;
    inline void setSrcAlphaBlendFactor( uint32_t attachmentIndex,
                                        VkBlendFactor newValue) noexcept;

    inline VkBlendFactor dstAlphaBlendFactor(
                                      uint32_t attachmentIndex) const noexcept;
    inline void setDstAlphaBlendFactor( uint32_t attachmentIndex,
                                        VkBlendFactor newValue) noexcept;

    inline VkBlendOp alphaBlendOp(uint32_t attachmentIndex) const noexcept;
    inline void setAlphaBlendOp(uint32_t attachmentIndex,
                                VkBlendOp newValue) noexcept;

    inline VkColorComponentFlags colorWriteMask(
                                        uint32_t attachmentIndex) const noexcept;
    inline void setColorWriteMask(uint32_t attachmentIndex,
                                  VkColorComponentFlags newValue) noexcept;

    inline bool blendLogicOpEnable() const noexcept;
    inline void setBlendLogicOpEnable(bool newValue) noexcept;

    inline VkLogicOp blendLogicOp() const noexcept;
    inline void setBlendLogicOp(VkLogicOp newValue) noexcept;

    inline glm::vec4 blendConstants() const noexcept;
    inline void setBlendConstants(const glm::vec4& newValue) noexcept;

  private:
    // Промежуточные данные, необходимые для построения конфигурации
    struct ShaderModuleInfo
    {
      std::unique_ptr<ShaderModule> shaderModule;
      VkShaderStageFlagBits stage;
    };
    // Промежуточные данные, необходимые для построения конфигурации
    struct ConfigurationBuildContext
    {
      std::vector<ShaderModuleInfo> shaders;
      ConstRef<PipelineLayout> pipelineLayout;
    };

  private:
    void _invalidateConfiguration() noexcept;
    void _buildConfiguration();
    void _processShader(const ShaderInfo& shaderRecord,
                        ConfigurationBuildContext& buildContext);
    void _processDescriptorSets(const ShaderInfo& shaderRecord,
                                const SpvReflectShaderModule& reflection);
    TechniqueConfiguration::DescriptorSet& _getOrCreateSet(
                                                        DescriptorSetType type);
    void _processBindings(const ShaderInfo& shaderRecord,
                          TechniqueConfiguration::DescriptorSet& set,
                          const SpvReflectDescriptorSet& reflectedSet);
    void _processUniformBlock(
                          const ShaderInfo& shaderRecord,
                          const SpvReflectDescriptorBinding& reflectedBinding);
    void _parseUniformBlockMember(
                            const ShaderInfo& shaderRecord,
                            TechniqueConfiguration::UniformBuffer& target,
                            const SpvReflectBlockVariable& sourceMember,
                            std::string namePrefix,
                            uint32_t parentBlockOffset);
    void _createLayouts(ConfigurationBuildContext& buildContext);
    void _createPipeline(ConfigurationBuildContext& buildContext);

  private:
    Device& _device;
    size_t _revision;

    Ref<TechniqueConfiguration> _configuration;
    bool _needRebuildConfiguration;

    AbstractPipeline::Type _pipelineType;

    std::vector<ShaderInfo> _shaders;

    // Настройки графического пайплайна. Игнорируются компьют пайплайном.
    std::optional<FrameBufferFormat> _frameBufferFormat;
    VkPrimitiveTopology _topology;
    VkPipelineRasterizationStateCreateInfo _rasterizationState;
    VkPipelineDepthStencilStateCreateInfo _depthStencilState;
    VkPipelineColorBlendStateCreateInfo _blendingState;
    std::array< VkPipelineColorBlendAttachmentState,
                FrameBufferFormat::maxColorAttachments> _attachmentsBlending;
  };

  inline Device& Technique::device() const noexcept
  {
    return _device;
  }

  inline size_t Technique::revision() const noexcept
  {
    return _revision;
  }

  inline const TechniqueConfiguration* Technique::configuration()
  {
    if(_needRebuildConfiguration) _buildConfiguration();
    MT_ASSERT(_needRebuildConfiguration == false);
    return _configuration.get();
  }

  inline void Technique::forceUpdate()
  {
    if (_needRebuildConfiguration) _buildConfiguration();
  }

  inline AbstractPipeline::Type Technique::pipelineType() const noexcept
  {
    return _pipelineType;
  }

  inline void Technique::setPipelineType(
                                      AbstractPipeline::Type newValue) noexcept
  {
    _pipelineType = newValue;
    _invalidateConfiguration();
  }

  inline const std::vector<Technique::ShaderInfo>&
                                            Technique::shaders() const noexcept
  {
    return _shaders;
  }

  inline void Technique::setShaders(std::span<const ShaderInfo> newShaders)
  {
    std::vector<ShaderInfo> newShadersTable(newShaders.begin(),
                                            newShaders.end());
    _shaders = std::move(newShadersTable);
    _invalidateConfiguration();
  }

  inline const FrameBufferFormat* Technique::frameBufferFormat() const noexcept
  {
    if(!_frameBufferFormat.has_value()) return nullptr;
    return &_frameBufferFormat.value();
  }

  inline const void Technique::setFrameBufferFormat(
                                  const FrameBufferFormat* newFormat) noexcept
  {
    if(newFormat == nullptr) _frameBufferFormat.reset();
    else
    {
      _frameBufferFormat = *newFormat;
      _blendingState.attachmentCount =
                        uint32_t(_frameBufferFormat->colorAttachments().size());
    }
    _invalidateConfiguration();
  }

  inline VkPrimitiveTopology Technique::topology() const noexcept
  {
    return _topology;
  }

  inline void Technique::setTopology(VkPrimitiveTopology newValue) noexcept
  {
    _topology = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::depthClampEnable() const noexcept
  {
    return _rasterizationState.depthClampEnable;
  }

  inline void Technique::setDepthClampEnable(bool newValue) noexcept
  {
    _rasterizationState.depthClampEnable = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::rasterizationDiscardEnable() const noexcept
  {
    return _rasterizationState.rasterizerDiscardEnable;
  }

  inline void Technique::setRasterizationDiscardEnable(bool newValue) noexcept
  {
    _rasterizationState.rasterizerDiscardEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkPolygonMode Technique::polygonMode() const noexcept
  {
    return _rasterizationState.polygonMode;
  }

  inline void Technique::setPolygonMode(VkPolygonMode newValue) noexcept
  {
    _rasterizationState.polygonMode = newValue;
    _invalidateConfiguration();
  }

  inline VkCullModeFlags Technique::cullMode() const noexcept
  {
    return _rasterizationState.cullMode;
  }

  inline void Technique::setCullMode(VkCullModeFlags newValue) noexcept
  {
    _rasterizationState.cullMode = newValue;
    _invalidateConfiguration();
  }

  inline VkFrontFace Technique::frontFace() const noexcept
  {
    return _rasterizationState.frontFace;
  }

  inline void Technique::setFrontFace(VkFrontFace newValue) noexcept
  {
    _rasterizationState.frontFace = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::depthBiasEnable() const noexcept
  {
    return _rasterizationState.depthBiasEnable;
  }

  inline void Technique::setDepthBiasEnable(bool newValue) noexcept
  {
    _rasterizationState.depthBiasEnable = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::depthBiasConstantFactor() const noexcept
  {
    return _rasterizationState.depthBiasConstantFactor;
  }

  inline void Technique::setDepthBiasConstantFactor(float newValue) noexcept
  {
    _rasterizationState.depthBiasConstantFactor = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::depthBiasClamp() const noexcept
  {
    return _rasterizationState.depthBiasClamp;
  }

  inline void Technique::setDepthBiasClamp(float newValue) noexcept
  {
    _rasterizationState.depthBiasClamp = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::depthBiasSlopeFactor() const noexcept
  {
    return _rasterizationState.depthBiasSlopeFactor;
  }

  inline void Technique::setDepthBiasSlopeFactor(float newValue) noexcept
  {
    _rasterizationState.depthBiasSlopeFactor = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::lineWidth() const noexcept
  {
    return _rasterizationState.lineWidth;
  }

  inline void Technique::setLineWidth(float newValue) noexcept
  {
    _rasterizationState.lineWidth = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::depthTestEnable() const noexcept
  {
    return _depthStencilState.depthTestEnable;
  }

  inline void Technique::setDepthTestEnable(bool newValue) noexcept
  {
    _depthStencilState.depthTestEnable = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::depthWriteEnable() const noexcept
  {
    return _depthStencilState.depthWriteEnable;
  }

  inline void Technique::setDepthWriteEnable(bool newValue) noexcept
  {
    _depthStencilState.depthWriteEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkCompareOp Technique::depthCompareOp() const noexcept
  {
    return _depthStencilState.depthCompareOp;
  }

  inline void Technique::setDepthCompareOp(VkCompareOp newValue) noexcept
  {
    _depthStencilState.depthCompareOp = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::depthBoundsTestEnable() const noexcept
  {
    return _depthStencilState.depthBoundsTestEnable;
  }

  inline void Technique::setDepthBoundsTestEnable(bool newValue) noexcept
  {
    _depthStencilState.depthBoundsTestEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkStencilOpState Technique::frontStencilOp() const noexcept
  {
    return _depthStencilState.front;
  }

  inline void Technique::setFrontStencilOp(VkStencilOpState newValue) noexcept
  {
    _depthStencilState.front = newValue;
    _invalidateConfiguration();
  }

  inline VkStencilOpState Technique::backStencilOp() const noexcept
  {
    return _depthStencilState.back;
  }

  inline void Technique::setBackStencilOp(VkStencilOpState newValue) noexcept
  {
    _depthStencilState.back = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::minDepthBounds() const noexcept
  {
    return _depthStencilState.minDepthBounds;
  }

  inline void Technique::setMinDepthBounds(float newValue) noexcept
  {
    _depthStencilState.minDepthBounds = newValue;
    _invalidateConfiguration();
  }

  inline float Technique::maxDepthBounds() const noexcept
  {
    return _depthStencilState.maxDepthBounds;
  }

  inline void Technique::setMaxDepthBounds(float newValue) noexcept
  {
    _depthStencilState.maxDepthBounds = newValue;
    _invalidateConfiguration();
  }

  inline bool Technique::blendLogicOpEnable() const noexcept
  {
    return _blendingState.logicOpEnable;
  }

  inline void Technique::setBlendLogicOpEnable(bool newValue) noexcept
  {
    _blendingState.logicOpEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkLogicOp Technique::blendLogicOp() const noexcept
  {
    return _blendingState.logicOp;
  }

  inline void Technique::setBlendLogicOp(VkLogicOp newValue) noexcept
  {
    _blendingState.logicOp = newValue;
    _invalidateConfiguration();
  }

  inline glm::vec4 Technique::blendConstants() const noexcept
  {
    return glm::vec4( _blendingState.blendConstants[0],
                      _blendingState.blendConstants[1],
                      _blendingState.blendConstants[2],
                      _blendingState.blendConstants[3]);
  }

  inline void Technique::setBlendConstants(const glm::vec4& newValue) noexcept
  {
    _blendingState.blendConstants[0] = newValue.r;
    _blendingState.blendConstants[0] = newValue.g;
    _blendingState.blendConstants[0] = newValue.b;
    _blendingState.blendConstants[0] = newValue.a;
    _invalidateConfiguration();
  }

  inline bool Technique::blendEnable(uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].blendEnable;
  }

  inline void Technique::setBlendEnable(uint32_t attachmentIndex,
                                        bool newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].blendEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor Technique::srcColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcColorBlendFactor;
  }

  inline void Technique::setSrcColorBlendFactor(uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcColorBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor Technique::dstColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstColorBlendFactor;
  }

  inline void Technique::setDstColorBlendFactor(uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstColorBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendOp Technique::colorBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorBlendOp;
  }

  inline void Technique::setColorBlendOp( uint32_t attachmentIndex,
                                          VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorBlendOp = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor Technique::srcAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor;
  }

  inline void Technique::setSrcAlphaBlendFactor(uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor Technique::dstAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor;
  }

  inline void Technique::setDstAlphaBlendFactor(uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendOp Technique::alphaBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].alphaBlendOp;
  }

  inline void Technique::setAlphaBlendOp( uint32_t attachmentIndex,
                                          VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].alphaBlendOp = newValue;
    _invalidateConfiguration();
  }

  inline VkColorComponentFlags Technique::colorWriteMask(
                                      uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorWriteMask;
  }

  inline void Technique::setColorWriteMask(
                                      uint32_t attachmentIndex,
                                      VkColorComponentFlags newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorWriteMask = newValue;
    _invalidateConfiguration();
  }
}