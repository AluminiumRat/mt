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
#include <technique/DescriptorSetType.h>
#include <technique/ShaderCompilator.h>
#include <technique/TechniqueConfiguration.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/FrameBufferFormat.h>

struct SpvReflectBlockVariable;
struct SpvReflectDescriptorBinding;
struct SpvReflectDescriptorSet;
struct SpvReflectShaderModule;

namespace mt
{
  class Device;

  // Класс для настройки конфигурации техник.
  class TechniqueConfigurator : public RefCounter
  {
  public:
    struct ShaderInfo
    {
      VkShaderStageFlagBits stage;
      std::string filename;
    };

    //  Дефайн в шейдере, который может принимать только ограниченное количество
    //  значений. Позволяет заранее скомпилировать все возможные вариации
    //  пайплайна и во время рендера просто выбирать нужный.
    struct SelectionDefine
    {
      std::string name;
      std::vector<std::string> valueVariants;
    };

  public:
    TechniqueConfigurator(Device& device, const char* debugName = "Technique") noexcept;
    TechniqueConfigurator(const TechniqueConfigurator&) = delete;
    TechniqueConfigurator& operator = (const TechniqueConfigurator&) = delete;
  protected:
    ~TechniqueConfigurator() noexcept = default;
  public:

    inline Device& device() const noexcept;

    //  Ревизия используется для отслеживания изменений в конфигурации
    //  Каждый раз, когда происходтит изменение состояния, ревизия увеличивается
    //  и зависимым объектам необходимо синхронизироваться с конфигурацией
    inline size_t revision() const noexcept;

    //  Может возвращать nullptr, если сборка конфигурации не завершилась
    //  успешно (например, ошибка в шейдере или неполный набор шейдеров)
    inline const TechniqueConfiguration* configuration();

    //  Если конфигурация ещё не инициализированa, то сделать это прямо сейчас
    //  Позволяет избежать задержек при первом использовании техники из-за
    //    ленивой инициализации.
    inline void forceUpdate();

    inline const std::string& debugName() const noexcept;

    inline AbstractPipeline::Type pipelineType() const noexcept;
    inline void setPipelineType(AbstractPipeline::Type newValue) noexcept;

    inline const std::vector<ShaderInfo>& shaders() const noexcept;
    inline void setShaders(std::span<const ShaderInfo> newShaders);

    inline const std::vector<SelectionDefine>& selections() const noexcept;
    inline void setSelections(std::span<const SelectionDefine> newSelections);

    //  Может возвращать nullptr, если формат не установлен (например, для
    //  компьют пайплайна)
    inline const FrameBufferFormat* frameBufferFormat() const noexcept;
    //  Можно передавать nullptr или не устанавливать формат для компьют техник
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
    //  Промежуточные данные, необходимые для построения конфигурации
    //  Один собранный шейдерный модуль
    struct ShaderModuleInfo
    {
      std::unique_ptr<ShaderModule> shaderModule;
      VkShaderStageFlagBits stage;
    };
    //  Набор шейдерных модулей для одного варианта пайплайна
    using ShaderSet = std::vector<ShaderModuleInfo>;

    // Промежуточные данные, необходимые для построения конфигурации
    struct ConfigurationBuildContext
    {
      std::vector<ShaderSet> shaders;
      ConstRef<PipelineLayout> pipelineLayout;
      uint32_t variantsNumber;
      uint32_t currentVariantIndex;
      std::vector<ShaderCompilator::Define> currentDefines;
    };

  private:
    void _invalidateConfiguration() noexcept;
    void _buildConfiguration();
    //  Посчитать количество вариантов пайплайна и веса селекшенов
    void _processSelections(ConfigurationBuildContext& buildContext);
    //  Обойти все варианты селекшенов и создать/обработать для них
    //  шейдерные модули
    //  selectionIndex - параметр для рекурсивного вызова
    void _processShadersSeveralVariants(
                                      ConfigurationBuildContext& buildContext,
                                      uint32_t selectionIndex);
    //  Создать и обработать набор шейдерных модулей для 1 варианта селекшенов
    void _processShadersOneVariant(ConfigurationBuildContext& buildContext);
    //  Создать и обработать отдельный шейдерный модуль
    void _processShader(const ShaderInfo& shaderRecord,
                        ConfigurationBuildContext& buildContext);
    //  Парсим рефлексию для отдельного шейдерного модуля
    void _processShaderReflection(const ShaderInfo& shaderRecord,
                                  const SpvReflectShaderModule& reflection);
    TechniqueConfiguration::DescriptorSet& _getOrCreateSet(
                                                        DescriptorSetType type);
    //  Обработка рефлексии
    void _processBindings(const ShaderInfo& shaderRecord,
                          TechniqueConfiguration::DescriptorSet& set,
                          const SpvReflectDescriptorSet& reflectedSet);
    //  Обработка рефлексии
    void _processUniformBlock(
                          const ShaderInfo& shaderRecord,
                          const SpvReflectDescriptorBinding& reflectedBinding);
    //  Обработка рефлексии. Разбираем отдельный фрагмент уноформ буфера
    void _parseUniformBlockMember(
                            const ShaderInfo& shaderRecord,
                            TechniqueConfiguration::UniformBuffer& target,
                            const SpvReflectBlockVariable& sourceMember,
                            std::string namePrefix,
                            uint32_t parentBlockOffset);
    //  По пропарсенным данным из рефлексии создаем лэйауты для
    //  дескриптер сетов и пайплайнов
    void _createLayouts(ConfigurationBuildContext& buildContext);
    //  Финальная стадия построения конфигурации - создаем все варианты
    //  пайплайнов
    void _createPipelines(ConfigurationBuildContext& buildContext);

  private:
    Device& _device;
    size_t _revision;

    std::string _debugName;

    Ref<TechniqueConfiguration> _configuration;
    bool _needRebuildConfiguration;

    AbstractPipeline::Type _pipelineType;

    std::vector<ShaderInfo> _shaders;
    std::vector<SelectionDefine> _selections;

    // Настройки графического пайплайна. Игнорируются компьют пайплайном.
    std::optional<FrameBufferFormat> _frameBufferFormat;
    VkPrimitiveTopology _topology;
    VkPipelineRasterizationStateCreateInfo _rasterizationState;
    VkPipelineDepthStencilStateCreateInfo _depthStencilState;
    VkPipelineColorBlendStateCreateInfo _blendingState;
    std::array< VkPipelineColorBlendAttachmentState,
                FrameBufferFormat::maxColorAttachments> _attachmentsBlending;
  };

  inline Device& TechniqueConfigurator::device() const noexcept
  {
    return _device;
  }

  inline size_t TechniqueConfigurator::revision() const noexcept
  {
    return _revision;
  }

  inline const TechniqueConfiguration* TechniqueConfigurator::configuration()
  {
    if(_needRebuildConfiguration) _buildConfiguration();
    MT_ASSERT(_needRebuildConfiguration == false);
    return _configuration.get();
  }

  inline void TechniqueConfigurator::forceUpdate()
  {
    if (_needRebuildConfiguration) _buildConfiguration();
  }

  inline const std::string& TechniqueConfigurator::debugName() const noexcept
  {
    return _debugName;
  }

  inline AbstractPipeline::Type TechniqueConfigurator::pipelineType()
                                                                  const noexcept
  {
    return _pipelineType;
  }

  inline void TechniqueConfigurator::setPipelineType(
                                      AbstractPipeline::Type newValue) noexcept
  {
    _pipelineType = newValue;
    _invalidateConfiguration();
  }

  inline const std::vector<TechniqueConfigurator::ShaderInfo>&
                                TechniqueConfigurator::shaders() const noexcept
  {
    return _shaders;
  }

  inline void TechniqueConfigurator::setShaders(
                                        std::span<const ShaderInfo> newShaders)
  {
    std::vector<ShaderInfo> newShadersTable(newShaders.begin(),
                                            newShaders.end());
    _shaders = std::move(newShadersTable);
    _invalidateConfiguration();
  }

  inline const std::vector<TechniqueConfigurator::SelectionDefine>&
                            TechniqueConfigurator::selections() const noexcept
  {
    return _selections;
  }

  inline void TechniqueConfigurator::setSelections(
                                std::span<const SelectionDefine> newSelections)
  {
    std::vector<SelectionDefine> newSelectionsVector( newSelections.begin(),
                                                      newSelections.end());
    _selections = std::move(newSelectionsVector);
    _invalidateConfiguration();
  }

  inline const FrameBufferFormat*
                      TechniqueConfigurator::frameBufferFormat() const noexcept
  {
    if(!_frameBufferFormat.has_value()) return nullptr;
    return &_frameBufferFormat.value();
  }

  inline const void TechniqueConfigurator::setFrameBufferFormat(
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

  inline VkPrimitiveTopology TechniqueConfigurator::topology() const noexcept
  {
    return _topology;
  }

  inline void TechniqueConfigurator::setTopology(
                                          VkPrimitiveTopology newValue) noexcept
  {
    _topology = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::depthClampEnable() const noexcept
  {
    return _rasterizationState.depthClampEnable;
  }

  inline void TechniqueConfigurator::setDepthClampEnable(bool newValue) noexcept
  {
    _rasterizationState.depthClampEnable = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::rasterizationDiscardEnable() const noexcept
  {
    return _rasterizationState.rasterizerDiscardEnable;
  }

  inline void TechniqueConfigurator::setRasterizationDiscardEnable(
                                                        bool newValue) noexcept
  {
    _rasterizationState.rasterizerDiscardEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkPolygonMode TechniqueConfigurator::polygonMode() const noexcept
  {
    return _rasterizationState.polygonMode;
  }

  inline void TechniqueConfigurator::setPolygonMode(
                                                VkPolygonMode newValue) noexcept
  {
    _rasterizationState.polygonMode = newValue;
    _invalidateConfiguration();
  }

  inline VkCullModeFlags TechniqueConfigurator::cullMode() const noexcept
  {
    return _rasterizationState.cullMode;
  }

  inline void TechniqueConfigurator::setCullMode(
                                              VkCullModeFlags newValue) noexcept
  {
    _rasterizationState.cullMode = newValue;
    _invalidateConfiguration();
  }

  inline VkFrontFace TechniqueConfigurator::frontFace() const noexcept
  {
    return _rasterizationState.frontFace;
  }

  inline void TechniqueConfigurator::setFrontFace(VkFrontFace newValue) noexcept
  {
    _rasterizationState.frontFace = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::depthBiasEnable() const noexcept
  {
    return _rasterizationState.depthBiasEnable;
  }

  inline void TechniqueConfigurator::setDepthBiasEnable(bool newValue) noexcept
  {
    _rasterizationState.depthBiasEnable = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::depthBiasConstantFactor() const noexcept
  {
    return _rasterizationState.depthBiasConstantFactor;
  }

  inline void TechniqueConfigurator::setDepthBiasConstantFactor(
                                                        float newValue) noexcept
  {
    _rasterizationState.depthBiasConstantFactor = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::depthBiasClamp() const noexcept
  {
    return _rasterizationState.depthBiasClamp;
  }

  inline void TechniqueConfigurator::setDepthBiasClamp(float newValue) noexcept
  {
    _rasterizationState.depthBiasClamp = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::depthBiasSlopeFactor() const noexcept
  {
    return _rasterizationState.depthBiasSlopeFactor;
  }

  inline void TechniqueConfigurator::setDepthBiasSlopeFactor(
                                                        float newValue) noexcept
  {
    _rasterizationState.depthBiasSlopeFactor = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::lineWidth() const noexcept
  {
    return _rasterizationState.lineWidth;
  }

  inline void TechniqueConfigurator::setLineWidth(float newValue) noexcept
  {
    _rasterizationState.lineWidth = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::depthTestEnable() const noexcept
  {
    return _depthStencilState.depthTestEnable;
  }

  inline void TechniqueConfigurator::setDepthTestEnable(bool newValue) noexcept
  {
    _depthStencilState.depthTestEnable = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::depthWriteEnable() const noexcept
  {
    return _depthStencilState.depthWriteEnable;
  }

  inline void TechniqueConfigurator::setDepthWriteEnable(bool newValue) noexcept
  {
    _depthStencilState.depthWriteEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkCompareOp TechniqueConfigurator::depthCompareOp() const noexcept
  {
    return _depthStencilState.depthCompareOp;
  }

  inline void TechniqueConfigurator::setDepthCompareOp(
                                                  VkCompareOp newValue) noexcept
  {
    _depthStencilState.depthCompareOp = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::depthBoundsTestEnable() const noexcept
  {
    return _depthStencilState.depthBoundsTestEnable;
  }

  inline void TechniqueConfigurator::setDepthBoundsTestEnable(
                                                        bool newValue) noexcept
  {
    _depthStencilState.depthBoundsTestEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkStencilOpState TechniqueConfigurator::frontStencilOp() const noexcept
  {
    return _depthStencilState.front;
  }

  inline void TechniqueConfigurator::setFrontStencilOp(
                                            VkStencilOpState newValue) noexcept
  {
    _depthStencilState.front = newValue;
    _invalidateConfiguration();
  }

  inline VkStencilOpState TechniqueConfigurator::backStencilOp() const noexcept
  {
    return _depthStencilState.back;
  }

  inline void TechniqueConfigurator::setBackStencilOp(
                                            VkStencilOpState newValue) noexcept
  {
    _depthStencilState.back = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::minDepthBounds() const noexcept
  {
    return _depthStencilState.minDepthBounds;
  }

  inline void TechniqueConfigurator::setMinDepthBounds(float newValue) noexcept
  {
    _depthStencilState.minDepthBounds = newValue;
    _invalidateConfiguration();
  }

  inline float TechniqueConfigurator::maxDepthBounds() const noexcept
  {
    return _depthStencilState.maxDepthBounds;
  }

  inline void TechniqueConfigurator::setMaxDepthBounds(float newValue) noexcept
  {
    _depthStencilState.maxDepthBounds = newValue;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::blendLogicOpEnable() const noexcept
  {
    return _blendingState.logicOpEnable;
  }

  inline void TechniqueConfigurator::setBlendLogicOpEnable(
                                                        bool newValue) noexcept
  {
    _blendingState.logicOpEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkLogicOp TechniqueConfigurator::blendLogicOp() const noexcept
  {
    return _blendingState.logicOp;
  }

  inline void TechniqueConfigurator::setBlendLogicOp(
                                                    VkLogicOp newValue) noexcept
  {
    _blendingState.logicOp = newValue;
    _invalidateConfiguration();
  }

  inline glm::vec4 TechniqueConfigurator::blendConstants() const noexcept
  {
    return glm::vec4( _blendingState.blendConstants[0],
                      _blendingState.blendConstants[1],
                      _blendingState.blendConstants[2],
                      _blendingState.blendConstants[3]);
  }

  inline void TechniqueConfigurator::setBlendConstants(
                                            const glm::vec4& newValue) noexcept
  {
    _blendingState.blendConstants[0] = newValue.r;
    _blendingState.blendConstants[0] = newValue.g;
    _blendingState.blendConstants[0] = newValue.b;
    _blendingState.blendConstants[0] = newValue.a;
    _invalidateConfiguration();
  }

  inline bool TechniqueConfigurator::blendEnable(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].blendEnable;
  }

  inline void TechniqueConfigurator::setBlendEnable(uint32_t attachmentIndex,
                                                    bool newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].blendEnable = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor TechniqueConfigurator::srcColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcColorBlendFactor;
  }

  inline void TechniqueConfigurator::setSrcColorBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcColorBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor TechniqueConfigurator::dstColorBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstColorBlendFactor;
  }

  inline void TechniqueConfigurator::setDstColorBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstColorBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendOp TechniqueConfigurator::colorBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorBlendOp;
  }

  inline void TechniqueConfigurator::setColorBlendOp(
                                                  uint32_t attachmentIndex,
                                                  VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorBlendOp = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor TechniqueConfigurator::srcAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor;
  }

  inline void TechniqueConfigurator::setSrcAlphaBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].srcAlphaBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendFactor TechniqueConfigurator::dstAlphaBlendFactor(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor;
  }

  inline void TechniqueConfigurator::setDstAlphaBlendFactor(
                                                uint32_t attachmentIndex,
                                                VkBlendFactor newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].dstAlphaBlendFactor = newValue;
    _invalidateConfiguration();
  }

  inline VkBlendOp TechniqueConfigurator::alphaBlendOp(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].alphaBlendOp;
  }

  inline void TechniqueConfigurator::setAlphaBlendOp(
                                                    uint32_t attachmentIndex,
                                                    VkBlendOp newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].alphaBlendOp = newValue;
    _invalidateConfiguration();
  }

  inline VkColorComponentFlags TechniqueConfigurator::colorWriteMask(
                                        uint32_t attachmentIndex) const noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    return _attachmentsBlending[attachmentIndex].colorWriteMask;
  }

  inline void TechniqueConfigurator::setColorWriteMask(
                                        uint32_t attachmentIndex,
                                        VkColorComponentFlags newValue) noexcept
  {
    MT_ASSERT(attachmentIndex < FrameBufferFormat::maxColorAttachments);
    _attachmentsBlending[attachmentIndex].colorWriteMask = newValue;
    _invalidateConfiguration();
  }
}