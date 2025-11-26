#pragma once

#include <span>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <technique/TechniqueConfiguration.h>
#include <util/Assert.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/FrameBufferFormat.h>

struct SpvReflectBlockVariable;
struct SpvReflectDescriptorBinding;
struct SpvReflectDescriptorSet;
struct SpvReflectShaderModule;

namespace mt
{
  struct ConfigurationBuildContext;

  //  Конфигуратор отдельного прохода для техники
  //  Составная часть TechniqueConfigurator
  class PassConfigurator
  {
  public:
    struct ShaderInfo
    {
      VkShaderStageFlagBits stage;
      std::string filename;
    };
    using Shaders = std::vector<ShaderInfo>;

  public:
    PassConfigurator(const char* name = "Pass");
    PassConfigurator(const PassConfigurator&) = delete;
    PassConfigurator& operator = (const PassConfigurator&) = delete;
    ~PassConfigurator() noexcept = default;

    inline const std::string& name() const noexcept;

    inline AbstractPipeline::Type pipelineType() const noexcept;
    inline void setPipelineType(AbstractPipeline::Type newValue) noexcept;

    inline const Shaders& shaders() const noexcept;
    inline void setShaders(std::span<const ShaderInfo> newShaders);

    //  Названия селекшенов, которые используются для этого прохода
    //  Сами селекшены и их варианты описываются в TechniqueConfigurator
    inline const std::vector<std::string>& selections() const noexcept;
    inline void setSelections(std::span<const std::string> newSelections);

    //  Может возвращать nullptr, если формат не установлен (например, для
    //  компьют пайплайна)
    inline const FrameBufferFormat* frameBufferFormat() const noexcept;
    //  Можно передавать nullptr или не устанавливать формат для компьют техник
    inline const void setFrameBufferFormat(
                                  const FrameBufferFormat* newFormat) noexcept;

    inline VkPrimitiveTopology topology() const noexcept;
    inline void setTopology(VkPrimitiveTopology newValue) noexcept;

    inline VkPolygonMode polygonMode() const noexcept;
    inline void setPolygonMode(VkPolygonMode newValue) noexcept;

    inline float lineWidth() const noexcept;
    inline void setLineWidth(float newValue) noexcept;

    inline VkCullModeFlags cullMode() const noexcept;
    inline void setCullMode(VkCullModeFlags newValue) noexcept;

    inline VkFrontFace frontFace() const noexcept;
    inline void setFrontFace(VkFrontFace newValue) noexcept;

    inline bool rasterizationDiscardEnable() const noexcept;
    inline void setRasterizationDiscardEnable(bool newValue) noexcept;

    //--------------------------
    //  Depth конфигурация
    inline bool depthTestEnable() const noexcept;
    inline void setDepthTestEnable(bool newValue) noexcept;

    inline bool depthWriteEnable() const noexcept;
    inline void setDepthWriteEnable(bool newValue) noexcept;

    inline VkCompareOp depthCompareOp() const noexcept;
    inline void setDepthCompareOp(VkCompareOp newValue) noexcept;

    inline bool depthClampEnable() const noexcept;
    inline void setDepthClampEnable(bool newValue) noexcept;

    inline bool depthBiasEnable() const noexcept;
    inline void setDepthBiasEnable(bool newValue) noexcept;

    inline float depthBiasConstantFactor() const noexcept;
    inline void setDepthBiasConstantFactor(float newValue) noexcept;

    inline float depthBiasSlopeFactor() const noexcept;
    inline void setDepthBiasSlopeFactor(float newValue) noexcept;

    inline float depthBiasClamp() const noexcept;
    inline void setDepthBiasClamp(float newValue) noexcept;

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
    friend class TechniqueConfigurator;
    //  Создать и обработать шейдерные модули для конфигурации
    void processShaders(ConfigurationBuildContext& context) const;
    //  Пайплайны создаются отдельным вызовом после того как будут пройдены
    //  все пасы и будут сформированы общие лэйауты
    void createPipelines(ConfigurationBuildContext& context) const;

  private:
    //  Посчитать количество вариантов пайплайна и веса селекшенов
    void _processSelections(ConfigurationBuildContext& context) const;
    //  Обойти все варианты селекшенов и создать/обработать для них
    //  шейдерные модули
    //  selectionIndex - параметр для рекурсивного вызова
    void _processShadersSeveralVariants(
                                      uint32_t selectionIndex,
                                      ConfigurationBuildContext& context) const;
    //  Создать и обработать набор шейдерных модулей для 1 варианта селекшенов
    void _processShadersOneVariant(ConfigurationBuildContext& context) const;
    //  Создать и обработать отдельный шейдерный модуль
    void _processShader(const ShaderInfo& shaderRecord,
                        ConfigurationBuildContext& context) const;
    //  Парсим рефлексию для отдельного шейдерного модуля
    void _processShaderReflection(const ShaderInfo& shaderRecord,
                                  const SpvReflectShaderModule& reflection,
                                  ConfigurationBuildContext& context) const;
    TechniqueConfiguration::DescriptorSet& _getOrCreateSet(
                                      DescriptorSetType type,
                                      ConfigurationBuildContext& context) const;
    //  Обработка рефлексии
    void _processBindings(const ShaderInfo& shaderRecord,
                          TechniqueConfiguration::DescriptorSet& set,
                          const SpvReflectDescriptorSet& reflectedSet,
                          ConfigurationBuildContext& context) const;
    //  Обработка рефлексии
    void _processUniformBlock(
                          const ShaderInfo& shaderRecord,
                          const SpvReflectDescriptorBinding& reflectedBinding,
                          ConfigurationBuildContext& context) const;
    //  Обработка рефлексии. Разбираем отдельный фрагмент уноформ буфера
    void _parseUniformBlockMember(
                            const ShaderInfo& shaderRecord,
                            TechniqueConfiguration::UniformBuffer& target,
                            const SpvReflectBlockVariable& sourceMember,
                            std::string namePrefix,
                            uint32_t parentBlockOffset) const;
  private:
    std::string _name;
    AbstractPipeline::Type _pipelineType;
    std::vector<ShaderInfo> _shaders;
    std::vector<std::string> _selections;

    // Настройки графического пайплайна. Игнорируются компьют пайплайном.
    std::optional<FrameBufferFormat> _frameBufferFormat;
    VkPrimitiveTopology _topology;
    VkPipelineRasterizationStateCreateInfo _rasterizationState;
    VkPipelineDepthStencilStateCreateInfo _depthStencilState;
    VkPipelineColorBlendStateCreateInfo _blendingState;
    std::array< VkPipelineColorBlendAttachmentState,
                FrameBufferFormat::maxColorAttachments> _attachmentsBlending;
  };

  inline const std::string& PassConfigurator::name() const noexcept
  {
    return _name;
  }

  inline AbstractPipeline::Type PassConfigurator::pipelineType() const noexcept
  {
    return _pipelineType;
  }

  inline void PassConfigurator::setPipelineType(
                                      AbstractPipeline::Type newValue) noexcept
  {
    _pipelineType = newValue;
  }

  inline const PassConfigurator::Shaders&
                                      PassConfigurator::shaders() const noexcept
  {
    return _shaders;
  }

  inline void PassConfigurator::setShaders(
                                        std::span<const ShaderInfo> newShaders)
  {
    Shaders newShadersTable(newShaders.begin(), newShaders.end());
    _shaders = std::move(newShadersTable);
  }

  inline const std::vector<std::string>&
                                  PassConfigurator::selections() const noexcept
  {
    return _selections;
  }

  inline void PassConfigurator::setSelections(
                                    std::span<const std::string> newSelections)
  {
    std::vector<std::string> newSelectionsVector( newSelections.begin(),
                                                  newSelections.end());
    _selections = std::move(newSelectionsVector);
  }

  inline const FrameBufferFormat*
                            PassConfigurator::frameBufferFormat() const noexcept
  {
    if(!_frameBufferFormat.has_value()) return nullptr;
    return &_frameBufferFormat.value();
  }

  inline const void PassConfigurator::setFrameBufferFormat(
                                  const FrameBufferFormat* newFormat) noexcept
  {
    if(newFormat == nullptr) _frameBufferFormat.reset();
    else
    {
      _frameBufferFormat = *newFormat;
      _blendingState.attachmentCount =
                        uint32_t(_frameBufferFormat->colorAttachments().size());
    }
  }

  inline VkPrimitiveTopology PassConfigurator::topology() const noexcept
  {
    return _topology;
  }

  inline void PassConfigurator::setTopology(
                                          VkPrimitiveTopology newValue) noexcept
  {
    _topology = newValue;
  }

  inline bool PassConfigurator::depthClampEnable() const noexcept
  {
    return _rasterizationState.depthClampEnable;
  }

  inline void PassConfigurator::setDepthClampEnable(bool newValue) noexcept
  {
    _rasterizationState.depthClampEnable = newValue;
  }

  inline bool PassConfigurator::rasterizationDiscardEnable() const noexcept
  {
    return _rasterizationState.rasterizerDiscardEnable;
  }

  inline void PassConfigurator::setRasterizationDiscardEnable(
                                                        bool newValue) noexcept
  {
    _rasterizationState.rasterizerDiscardEnable = newValue;
  }

  inline VkPolygonMode PassConfigurator::polygonMode() const noexcept
  {
    return _rasterizationState.polygonMode;
  }

  inline void PassConfigurator::setPolygonMode(VkPolygonMode newValue) noexcept
  {
    _rasterizationState.polygonMode = newValue;
  }

  inline VkCullModeFlags PassConfigurator::cullMode() const noexcept
  {
    return _rasterizationState.cullMode;
  }

  inline void PassConfigurator::setCullMode(VkCullModeFlags newValue) noexcept
  {
    _rasterizationState.cullMode = newValue;
  }

  inline VkFrontFace PassConfigurator::frontFace() const noexcept
  {
    return _rasterizationState.frontFace;
  }

  inline void PassConfigurator::setFrontFace(VkFrontFace newValue) noexcept
  {
    _rasterizationState.frontFace = newValue;
  }

  inline bool PassConfigurator::depthBiasEnable() const noexcept
  {
    return _rasterizationState.depthBiasEnable;
  }

  inline void PassConfigurator::setDepthBiasEnable(bool newValue) noexcept
  {
    _rasterizationState.depthBiasEnable = newValue;
  }

  inline float PassConfigurator::depthBiasConstantFactor() const noexcept
  {
    return _rasterizationState.depthBiasConstantFactor;
  }

  inline void PassConfigurator::setDepthBiasConstantFactor(
                                                        float newValue) noexcept
  {
    _rasterizationState.depthBiasConstantFactor = newValue;
  }

  inline float PassConfigurator::depthBiasClamp() const noexcept
  {
    return _rasterizationState.depthBiasClamp;
  }

  inline void PassConfigurator::setDepthBiasClamp(float newValue) noexcept
  {
    _rasterizationState.depthBiasClamp = newValue;
  }

  inline float PassConfigurator::depthBiasSlopeFactor() const noexcept
  {
    return _rasterizationState.depthBiasSlopeFactor;
  }

  inline void PassConfigurator::setDepthBiasSlopeFactor(float newValue) noexcept
  {
    _rasterizationState.depthBiasSlopeFactor = newValue;
  }

  inline float PassConfigurator::lineWidth() const noexcept
  {
    return _rasterizationState.lineWidth;
  }

  inline void PassConfigurator::setLineWidth(float newValue) noexcept
  {
    _rasterizationState.lineWidth = newValue;
  }

  inline bool PassConfigurator::depthTestEnable() const noexcept
  {
    return _depthStencilState.depthTestEnable;
  }

  inline void PassConfigurator::setDepthTestEnable(bool newValue) noexcept
  {
    _depthStencilState.depthTestEnable = newValue;
  }

  inline bool PassConfigurator::depthWriteEnable() const noexcept
  {
    return _depthStencilState.depthWriteEnable;
  }

  inline void PassConfigurator::setDepthWriteEnable(bool newValue) noexcept
  {
    _depthStencilState.depthWriteEnable = newValue;
  }

  inline VkCompareOp PassConfigurator::depthCompareOp() const noexcept
  {
    return _depthStencilState.depthCompareOp;
  }

  inline void PassConfigurator::setDepthCompareOp(VkCompareOp newValue) noexcept
  {
    _depthStencilState.depthCompareOp = newValue;
  }

  inline bool PassConfigurator::depthBoundsTestEnable() const noexcept
  {
    return _depthStencilState.depthBoundsTestEnable;
  }

  inline void PassConfigurator::setDepthBoundsTestEnable(
                                                        bool newValue) noexcept
  {
    _depthStencilState.depthBoundsTestEnable = newValue;
  }

  inline VkStencilOpState PassConfigurator::frontStencilOp() const noexcept
  {
    return _depthStencilState.front;
  }

  inline void PassConfigurator::setFrontStencilOp(
                                            VkStencilOpState newValue) noexcept
  {
    _depthStencilState.front = newValue;
  }

  inline VkStencilOpState PassConfigurator::backStencilOp() const noexcept
  {
    return _depthStencilState.back;
  }

  inline void PassConfigurator::setBackStencilOp(
                                            VkStencilOpState newValue) noexcept
  {
    _depthStencilState.back = newValue;
  }

  inline float PassConfigurator::minDepthBounds() const noexcept
  {
    return _depthStencilState.minDepthBounds;
  }

  inline void PassConfigurator::setMinDepthBounds(float newValue) noexcept
  {
    _depthStencilState.minDepthBounds = newValue;
  }

  inline float PassConfigurator::maxDepthBounds() const noexcept
  {
    return _depthStencilState.maxDepthBounds;
  }

  inline void PassConfigurator::setMaxDepthBounds(float newValue) noexcept
  {
    _depthStencilState.maxDepthBounds = newValue;
  }

  inline bool PassConfigurator::blendLogicOpEnable() const noexcept
  {
    return _blendingState.logicOpEnable;
  }

  inline void PassConfigurator::setBlendLogicOpEnable(bool newValue) noexcept
  {
    _blendingState.logicOpEnable = newValue;
  }

  inline VkLogicOp PassConfigurator::blendLogicOp() const noexcept
  {
    return _blendingState.logicOp;
  }

  inline void PassConfigurator::setBlendLogicOp(
                                                    VkLogicOp newValue) noexcept
  {
    _blendingState.logicOp = newValue;
  }

  inline glm::vec4 PassConfigurator::blendConstants() const noexcept
  {
    return glm::vec4( _blendingState.blendConstants[0],
                      _blendingState.blendConstants[1],
                      _blendingState.blendConstants[2],
                      _blendingState.blendConstants[3]);
  }

  inline void PassConfigurator::setBlendConstants(
                                            const glm::vec4& newValue) noexcept
  {
    _blendingState.blendConstants[0] = newValue.r;
    _blendingState.blendConstants[0] = newValue.g;
    _blendingState.blendConstants[0] = newValue.b;
    _blendingState.blendConstants[0] = newValue.a;
  }

  inline bool PassConfigurator::blendEnable(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].blendEnable;
  }

  inline void PassConfigurator::setBlendEnable(uint32_t attachmentIndex,
                                                    bool newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].blendEnable = newValue;
  }

  inline VkBlendFactor PassConfigurator::srcColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcColorBlendFactor;
  }

  inline void PassConfigurator::setSrcColorBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcColorBlendFactor = newValue;
  }

  inline VkBlendFactor PassConfigurator::dstColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstColorBlendFactor;
  }

  inline void PassConfigurator::setDstColorBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstColorBlendFactor = newValue;
  }

  inline VkBlendOp PassConfigurator::colorBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorBlendOp;
  }

  inline void PassConfigurator::setColorBlendOp(uint32_t attachmentIndex,
                                                VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorBlendOp = newValue;
  }

  inline VkBlendFactor PassConfigurator::srcAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor;
  }

  inline void PassConfigurator::setSrcAlphaBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor = newValue;
  }

  inline VkBlendFactor PassConfigurator::dstAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor;
  }

  inline void PassConfigurator::setDstAlphaBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor = newValue;
  }

  inline VkBlendOp PassConfigurator::alphaBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].alphaBlendOp;
  }

  inline void PassConfigurator::setAlphaBlendOp(uint32_t attachmentIndex,
                                                VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].alphaBlendOp = newValue;
  }

  inline VkColorComponentFlags PassConfigurator::colorWriteMask(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorWriteMask;
  }

  inline void PassConfigurator::setColorWriteMask(
                                        uint32_t attachmentIndex,
                                        VkColorComponentFlags newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorWriteMask = newValue;
  }
};