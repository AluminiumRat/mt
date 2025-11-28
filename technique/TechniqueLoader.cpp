#include <stdexcept>
#include <vector>

#include <yaml-cpp/yaml.h>

#include <technique/TechniqueConfigurator.h>
#include <technique/TechniqueLoader.h>
#include <util/vkMeta.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/pipeline/ShaderLoader.h>

namespace mt
{
  struct FrameBufferDescription
  {
    std::vector<VkFormat> colorTargets;
    VkFormat depthTarget = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  };

  struct SamplerSettings
  {
    VkFilter magFilter = VK_FILTER_NEAREST;
    VkFilter minFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float mipLodBias = 0;
    bool anisotropyEnable = false;
    float maxAnisotropy = 1;
    bool compareEnable = false;
    VkCompareOp compareOp = VK_COMPARE_OP_NEVER;
    float minLod = 0;
    float maxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    bool unnormalizedCoordinates = false;
  };

  //  Загрузить настройки техники из отдельной YAML ноды
  static void loadConfigurator(TechniqueConfigurator& target, YAML::Node node);
  // Загрузить список селекшенов для отдельного прохода
  static void loadSelectionsList(YAML::Node passNode, PassConfigurator& target);
  //  Загрузить список селекшенов конфигурации
  static TechniqueConfigurator::Selections loadSelections(
                                                      YAML::Node selectionsNode);
  //  Загрузить один отдельный проход
  static void loadPass(YAML::Node passNode, PassConfigurator& target);
  static void loadShaders(YAML::Node passNode, PassConfigurator& target);
  static void updateFrameBufferFormat(YAML::Node frameBufferNode,
                                      FrameBufferDescription& target,
                                      const std::string& passName);
  static void loadGraphicSettings(YAML::Node settingsNode,
                                  PassConfigurator& target);
  static void updateStencilOp(YAML::Node opNode, VkStencilOpState& target);
  static void loadBlending(YAML::Node blendingNode, PassConfigurator& target);
  static void loadBlending( YAML::Node blendingNode,
                            PassConfigurator& target,
                            uint32_t attachmentIndex);
  static VkBlendFactor getBlendFactor(YAML::Node settings, const char* name);
  static VkBlendOp getBlendOp(YAML::Node settings, const char* name);
  static void loadDefaultSamplers(YAML::Node techniqueNode,
                                  TechniqueConfigurator& target);
  //  Загрузить настройки сэмплера
  static void updateSamplerSettings(YAML::Node samplerNode,
                                    SamplerSettings& target);
  static std::vector<std::string> getInheritanceSources(YAML::Node optionsNode);

  YAML::Node readFile(const char* filename)
  {
    ShaderLoader& textLoader = ShaderLoader::getShaderLoader();
    std::string fileText = textLoader.loadText(filename);
    YAML::Node config = YAML::Load(fileText);
    return config;
  }

  void loadConfigurator(TechniqueConfigurator& target, const char* filename)
  {
    try
    {
      loadConfigurator(target, readFile(filename));
    }
    catch (const std::runtime_error& error)
    {
      throw std::runtime_error(std::string(filename) + ": " + error.what());
    }
  }

  static void loadConfigurator(TechniqueConfigurator& target, YAML::Node node)
  {
    target.clear();
    try
    {
      // Загружаем селекшены
      YAML::Node selectionsNode = node["selections"];
      TechniqueConfigurator::Selections selections = loadSelections(
                                                                selectionsNode);
      target.setSelections(selections);

      // Загружаем проходы
      YAML::Node passesNode = node["passes"];
      if(passesNode.Type() != YAML::NodeType::Map) throw std::runtime_error("unable to parse 'passes' list");
      for(YAML::const_iterator iPass = passesNode.begin();
          iPass != passesNode.end();
          iPass++)
      {
        std::string passName = iPass->first.as<std::string>();
        std::unique_ptr<PassConfigurator> newPass(
                                      new PassConfigurator(passName.c_str()));
        loadPass(iPass->second, *newPass);
        target.addPass(std::move(newPass));
      }

      loadDefaultSamplers(node, target);
    }
    catch(...)
    {
      target.clear();
      throw;
    }
  }

  static TechniqueConfigurator::Selections loadSelections(
                                                      YAML::Node selectionsNode)
  {
    if (selectionsNode.Type() != YAML::NodeType::Map) return {};

    std::vector<TechniqueConfigurator::SelectionDefine> selections;
    selections.reserve(selectionsNode.size());

    for ( YAML::const_iterator iSelection = selectionsNode.begin();
          iSelection != selectionsNode.end();
          iSelection++)
    {
      std::string selectionName = iSelection->first.as<std::string>();

      YAML::Node variantsNode = iSelection->second;
      if(!variantsNode.IsSequence()) throw std::runtime_error("unable to parse variants list for selection " + selectionName);
      std::vector<std::string> variants;
      for(YAML::Node variantNode : variantsNode)
      {
        variants.push_back(variantNode.as<std::string>());
      }

      selections.push_back({.name = selectionName,
                            .valueVariants = variants });
    }

    return selections;
  }

  static void loadPass(YAML::Node passNode, PassConfigurator& target)
  {
    if(!passNode.IsMap()) throw std::runtime_error(target.name() + ": unable to parse pass configuration");

    std::string pipeline = passNode["pipeline"].as<std::string>("GRAPHIC");
    target.setPipelineType(pipelineMap[pipeline]);

    loadSelectionsList(passNode, target);
    loadShaders(passNode, target);
    if (target.pipelineType() == AbstractPipeline::GRAPHIC_PIPELINE)
    {
      //  Загружаем настройки фрэйм буфера
      FrameBufferDescription frameBufferDescription;
      updateFrameBufferFormat(passNode["frameBuffer"],
                              frameBufferDescription,
                              target.name());
      if(frameBufferDescription.colorTargets.empty()) throw std::runtime_error(target.name() + ": unable to find 'color' targets in 'frameBuffer' description");
      FrameBufferFormat fbFormat( frameBufferDescription.colorTargets,
                                  frameBufferDescription.depthTarget,
                                  frameBufferDescription.samples);
      target.setFrameBufferFormat(&fbFormat);

      // Настройки фиксированных стадий пайплайна
      loadGraphicSettings(passNode["graphicSettings"], target);
    }
  }

  static void loadSelectionsList(YAML::Node passNode, PassConfigurator& target)
  {
    YAML::Node selectionsNode = passNode["selections"];
    if (!selectionsNode.IsSequence()) return;

    std::vector<std::string> selections;
    for(YAML::Node selection : selectionsNode)
    {
      selections.push_back(selection.as<std::string>());
    }
    target.setSelections(selections);
  }

  static void loadShaders(YAML::Node passNode, PassConfigurator& target)
  {
    YAML::Node shadersNode = passNode["shaders"];
    if(!shadersNode.IsMap())  throw std::runtime_error(target.name() + ": unable to parse 'shaders' list");

    PassConfigurator::Shaders shaders;
    for ( YAML::const_iterator iShader = shadersNode.begin();
          iShader != shadersNode.end();
          iShader++)
    {
      std::string shaderStage = iShader->first.as<std::string>();
      std::string shaderFile = iShader->second.as<std::string>();
      shaders.push_back({ .stage = shaderStageMap[shaderStage],
                          .filename = shaderFile});
    }

    target.setShaders(shaders);
  }

  // Загрузить настройки фрэйм буфера и объединить их с тем, что есть в target
  static void updateFrameBufferFormat(YAML::Node frameBufferNode,
                                      FrameBufferDescription& target,
                                      const std::string& passName)
  {
    if (!frameBufferNode.IsMap())  throw std::runtime_error(passName + ": unable to parse 'frameBuffer' description");

    // Для начала смотрим, надо ли наследовать настройки от кого-нибудь
    std::vector<std::string> parents = getInheritanceSources(frameBufferNode);
    for(const std::string& filename : parents)
    {
      try
      {
        YAML::Node config = readFile(filename.c_str());
        updateFrameBufferFormat(config, target, passName);
      }
      catch(std::runtime_error error)
      {
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Парсим цветовые таргеты
    YAML::Node colorNode = frameBufferNode["color"];
    // color может быть как отдельным форматом, так и массивом форматов
    if(colorNode.IsScalar())
    {
      std::string formatName = colorNode.as<std::string>();
      target.colorTargets = {getFormatFeatures(formatName).format};
    }
    else if(colorNode.IsSequence())
    {
      if(colorNode.size() > FrameBufferFormat::maxColorAttachments) throw std::runtime_error(passName + ": frameBuffer: too many targets in 'color' array");
      target.colorTargets.clear();
      for(YAML::Node formatNode : colorNode)
      {
        std::string formatName = formatNode.as<std::string>();
        target.colorTargets.push_back(getFormatFeatures(formatName).format);
      }
    }

    // Парсим деф-стенсил
    YAML::Node depthStencilNode = frameBufferNode["depthStencil"];
    if(depthStencilNode.IsScalar())
    {
      std::string formatName = depthStencilNode.as<std::string>();
      target.depthTarget = getFormatFeatures(formatName).format;
    }

    // Мультисэмплинг
    YAML::Node samplesNode = frameBufferNode["samples"];
    if(samplesNode.IsScalar())
    {
      target.samples = sampleCountMap[samplesNode.as<std::string>("1")];
    }
  }

  static void loadGraphicSettings(YAML::Node settingsNode,
                                  PassConfigurator& target)
  {
    if (!settingsNode.IsMap()) return;

    // Для начала прогружаем унаследованные настройки
    std::vector<std::string> parents = getInheritanceSources(settingsNode);
    for (const std::string& filename : parents)
    {
      try
      {
        YAML::Node config = readFile(filename.c_str());
        loadGraphicSettings(config, target);
      }
      catch (std::runtime_error error)
      {
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Грузим собственные  настройки

    if(settingsNode["topology"].IsScalar())
    {
      std::string valueStr =
                      settingsNode["topology"].as<std::string>("TRIANGLE_LIST");
      target.setTopology(topologyMap[valueStr]);
    }

    if (settingsNode["polygonMode"].IsScalar())
    {
      std::string valueStr =
                            settingsNode["polygonMode"].as<std::string>("FILL");
      target.setPolygonMode(polygonModeMap[valueStr]);
    }

    if (settingsNode["lineWidth"].IsScalar())
    {
      target.setLineWidth(settingsNode["lineWidth"].as<float>(1.0f));
    }

    if (settingsNode["cullMode"].IsScalar())
    {
      std::string valueStr = settingsNode["cullMode"].as<std::string>("NONE");
      target.setCullMode(cullModeMap[valueStr]);
    }

    if (settingsNode["frontFace"].IsScalar())
    {
      std::string valueStr =
                settingsNode["frontFace"].as<std::string>("COUNTER_CLOCKWISE");
      target.setFrontFace(fronFaceMap[valueStr]);
    }

    if (settingsNode["rasterizationDiscardEnable"].IsScalar())
    {
      target.setRasterizationDiscardEnable(
                    settingsNode["rasterizationDiscardEnable"].as<bool>(false));
    }

    if (settingsNode["depthTest"].IsScalar())
    {
      target.setDepthTestEnable(settingsNode["depthTest"].as<bool>(false));
    }

    if (settingsNode["depthWrite"].IsScalar())
    {
      target.setDepthWriteEnable(settingsNode["depthWrite"].as<bool>(false));
    }

    if (settingsNode["depthCompareOp"].IsScalar())
    {
      std::string valueStr =
                        settingsNode["depthCompareOp"].as<std::string>("NEVER");
      target.setDepthCompareOp(compareOpMap[valueStr]);
    }

    if (settingsNode["depthClampEnable"].IsScalar())
    {
      target.setDepthClampEnable(
                              settingsNode["depthClampEnable"].as<bool>(false));
    }

    if (settingsNode["depthBias"].IsScalar())
    {
      target.setDepthBiasEnable(settingsNode["depthBias"].as<bool>(false));
    }

    if (settingsNode["depthBiasConstant"].IsScalar())
    {
      target.setDepthBiasConstantFactor(
                            settingsNode["depthBiasConstant"].as<float>(0.0f));
    }

    if (settingsNode["depthBiasSlope"].IsScalar())
    {
      target.setDepthBiasSlopeFactor(
                                settingsNode["depthBiasSlope"].as<float>(0.0f));
    }

    if (settingsNode["depthBiasClamp"].IsScalar())
    {
      target.setDepthBiasClamp(settingsNode["depthBiasClamp"].as<bool>(false));
    }

    if (settingsNode["depthBoundsTest"].IsScalar())
    {
      target.setDepthBoundsTestEnable(
                              settingsNode["depthBoundsTest"].as<bool>(false));
    }

    if (settingsNode["minDepthBounds"].IsScalar())
    {
      target.setMinDepthBounds(settingsNode["minDepthBounds"].as<float>(0.0f));
    }

    if (settingsNode["maxDepthBounds"].IsScalar())
    {
      target.setMaxDepthBounds(settingsNode["maxDepthBounds"].as<float>(0.0f));
    }

    if (settingsNode["frontStencilOp"].IsMap())
    {
      VkStencilOpState stencilSettings;
      updateStencilOp(settingsNode["frontStencilOp"], stencilSettings);
      target.setFrontStencilOp(stencilSettings);
    }

    if (settingsNode["backStencilOp"].IsMap())
    {
      VkStencilOpState stencilSettings;
      updateStencilOp(settingsNode["backStencilOp"], stencilSettings);
      target.setBackStencilOp(stencilSettings);
    }

    loadBlending(settingsNode["blending"], target);

    // Логические операции
    YAML::Node logicOp = settingsNode["blendLogicOp"];
    if(logicOp.IsScalar())
    {
      target.setBlendLogicOp(logicOpMap[logicOp.as<std::string>("")]);
    }

    // Константы блэндинга
    YAML::Node constants = settingsNode["blendConstants"];
    if(constants.IsSequence())
    {
      glm::vec4 newValue(0);
      int index = 0;
      for(YAML::Node component : constants)
      {
        newValue[index] = component.as<float>(0.0f);
        index++;
        if(index == 4) break;
      }
      target.setBlendConstants(newValue);
    }
  }

  static void updateStencilOp(YAML::Node opNode, VkStencilOpState& target)
  {
    if(!opNode.IsMap()) return;

    // Для начала прогружаем унаследованные настройки
    std::vector<std::string> parents = getInheritanceSources(opNode);
    for (const std::string& filename : parents)
    {
      try
      {
        YAML::Node config = readFile(filename.c_str());
        updateStencilOp(config, target);
      }
      catch (std::runtime_error error)
      {
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Собственные настройки

    if(opNode["failOp"].IsScalar())
    {
      std::string valueStr = opNode["failOp"].as<std::string>("KEEP");
      target.failOp = stencilOpMap[valueStr];
    }

    if (opNode["passOp"].IsScalar())
    {
      std::string valueStr = opNode["passOp"].as<std::string>("KEEP");
      target.passOp = stencilOpMap[valueStr];
    }

    if (opNode["depthFailOp"].IsScalar())
    {
      std::string valueStr = opNode["depthFailOp"].as<std::string>("KEEP");
      target.depthFailOp = stencilOpMap[valueStr];
    }

    if (opNode["compareOp"].IsScalar())
    {
      std::string valueStr = opNode["compareOp"].as<std::string>("NEVER");
      target.compareOp = compareOpMap[valueStr];
    }

    if (opNode["compareMask"].IsScalar())
    {
      target.compareMask = opNode["compareMask"].as<uint32_t>(0);
    }

    if (opNode["writeMask"].IsScalar())
    {
      target.writeMask = opNode["writeMask"].as<uint32_t>(0);
    }

    if (opNode["reference"].IsScalar())
    {
      target.reference = opNode["reference"].as<uint32_t>(0);
    }
  }

  static void loadBlending(YAML::Node blendingNode, PassConfigurator& target)
  {
    // Для начала прогружаем унаследованные настройки
    std::vector<std::string> parents = getInheritanceSources(blendingNode);
    for (const std::string& filename : parents)
    {
      try
      {
        YAML::Node config = readFile(filename.c_str());
        loadBlending(config, target);
      }
      catch (std::runtime_error error)
      {
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Собственные настройки
    if(blendingNode.IsSequence())
    {
      //  Раздельные настройки для разных таргетов
      uint32_t attachment = 0;
      for(YAML::Node subnode : blendingNode)
      {
        loadBlending(subnode, target, attachment);
        attachment++;
        if (attachment == FrameBufferFormat::maxColorAttachments) break;
      }
    }
    else if(blendingNode.IsMap())
    {
      //  Одна настройка на все таргеты
      for(uint32_t attachment = 0;
          attachment < target.frameBufferFormat()->colorAttachments().size();
          attachment++)
      {
        loadBlending(blendingNode, target, attachment);
      }
    }
  }

  static void loadBlending( YAML::Node blendingNode,
                            PassConfigurator& target,
                            uint32_t attachmentIndex)
  {
    if(!blendingNode.IsMap()) throw std::runtime_error(target.name() + ": unable to parse 'blending' description");

    if(blendingNode["blendEnable"].IsScalar())
    {
      target.setBlendEnable(attachmentIndex,
                            blendingNode["blendEnable"].as<bool>(false));
    }

    if (blendingNode["srcColorBlendFactor"].IsScalar())
    {
      target.setSrcColorBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "srcColorBlendFactor"));
    }

    if (blendingNode["dstColorBlendFactor"].IsScalar())
    {
      target.setDstColorBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "dstColorBlendFactor"));
    }

    if (blendingNode["colorBlendOp"].IsScalar())
    {
      target.setColorBlendOp( attachmentIndex,
                              getBlendOp(blendingNode, "colorBlendOp"));
    }

    if (blendingNode["srcAlphaBlendFactor"].IsScalar())
    {
      target.setSrcAlphaBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "srcAlphaBlendFactor"));
    }

    if (blendingNode["dstAlphaBlendFactor"].IsScalar())
    {
      target.setDstAlphaBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "dstAlphaBlendFactor"));
    }

    if (blendingNode["alphaBlendOp"].IsScalar())
    {
      target.setAlphaBlendOp( attachmentIndex,
                              getBlendOp(blendingNode, "alphaBlendOp"));
    }

    // Маска каналов
    YAML::Node maskNode = blendingNode["colorWriteMask"];
    if(maskNode.IsSequence())
    {
      VkColorComponentFlags writeMask = 0;
      for(YAML::Node channelNode : maskNode)
      {
        writeMask |= colorComponentMap[channelNode.as<std::string>("")];
      }
      target.setColorWriteMask(attachmentIndex, writeMask);
    }
  }

  static VkBlendFactor getBlendFactor(YAML::Node settings, const char* name)
  {
    std::string valueStr = settings[name].as<std::string>("ZERO");
    return blendFactorMap[valueStr];
  }

  static VkBlendOp getBlendOp(YAML::Node settings, const char* name)
  {
    std::string valueStr = settings[name].as<std::string>("ADD");
    return blendOpMap[valueStr];
  }

  static void loadDefaultSamplers(YAML::Node techniqueNode,
                                  TechniqueConfigurator& target)
  {
    YAML::Node listNode = techniqueNode["defaultSamplers"];
    if(!listNode.IsMap()) return;

    std::vector<TechniqueConfiguration::DefaultSampler> samplers;

    for(YAML::const_iterator iSampler = listNode.begin();
        iSampler != listNode.end();
        iSampler++)
    {
      TechniqueConfiguration::DefaultSampler sampler;
      sampler.resourceName = iSampler->first.as<std::string>("");

      SamplerSettings samplerSettings;
      updateSamplerSettings(iSampler->second, samplerSettings);

      sampler.defaultSampler =
                      ConstRef(new Sampler(
                                      target.device(),
                                      samplerSettings.magFilter,
                                      samplerSettings.minFilter,
                                      samplerSettings.mipmapMode,
                                      samplerSettings.addressModeU,
                                      samplerSettings.addressModeV,
                                      samplerSettings.addressModeW,
                                      samplerSettings.mipLodBias,
                                      samplerSettings.anisotropyEnable,
                                      samplerSettings.maxAnisotropy,
                                      samplerSettings.compareEnable,
                                      samplerSettings.compareOp,
                                      samplerSettings.minLod,
                                      samplerSettings.maxLod,
                                      samplerSettings.borderColor,
                                      samplerSettings.unnormalizedCoordinates));
      samplers.push_back(sampler);
    }

    target.setDefaultSamplers(samplers);
  }

  static void updateSamplerSettings(YAML::Node samplerNode,
                                    SamplerSettings& target)
  {
    // Для начала прогружаем унаследованные настройки
    std::vector<std::string> parents = getInheritanceSources(samplerNode);
    for (const std::string& filename : parents)
    {
      try
      {
        YAML::Node config = readFile(filename.c_str());
        updateSamplerSettings(config, target);
      }
      catch (std::runtime_error error)
      {
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Дальше грузим собственные настройки
    if(samplerNode["magFilter"].IsScalar())
    {
      std::string valueStr =
                            samplerNode["magFilter"].as<std::string>("NEAREST");
      target.magFilter = filterMap[valueStr];
    }

    if (samplerNode["minFilter"].IsScalar())
    {
      std::string valueStr =
                            samplerNode["minFilter"].as<std::string>("NEAREST");
      target.minFilter = filterMap[valueStr];
    }

    if (samplerNode["mipmapMode"].IsScalar())
    {
      std::string valueStr =
                          samplerNode["mipmapMode"].as<std::string>("NEAREST");
      target.mipmapMode = mipmapModeMap[valueStr];
    }

    if (samplerNode["addressModeU"].IsScalar())
    {
      std::string valueStr =
                        samplerNode["addressModeU"].as<std::string>("REPEAT");
      target.addressModeU = addressModeMap[valueStr];
    }

    if (samplerNode["addressModeV"].IsScalar())
    {
      std::string valueStr =
                        samplerNode["addressModeV"].as<std::string>("REPEAT");
      target.addressModeV = addressModeMap[valueStr];
    }

    if (samplerNode["addressModeW"].IsScalar())
    {
      std::string valueStr =
                        samplerNode["addressModeW"].as<std::string>("REPEAT");
      target.addressModeW = addressModeMap[valueStr];
    }

    if (samplerNode["mipLodBias"].IsScalar())
    {
      target.mipLodBias = samplerNode["mipLodBias"].as<float>(0.0f);
    }

    if (samplerNode["anisotropyEnable"].IsScalar())
    {
      target.anisotropyEnable = samplerNode["anisotropyEnable"].as<bool>(false);
    }

    if (samplerNode["maxAnisotropy"].IsScalar())
    {
      target.maxAnisotropy = samplerNode["maxAnisotropy"].as<float>(1.0f);
    }

    if (samplerNode["compareEnable"].IsScalar())
    {
      target.compareEnable = samplerNode["compareEnable"].as<bool>(false);
    }

    if (samplerNode["compareOp"].IsScalar())
    {
      std::string valueStr = samplerNode["compareOp"].as<std::string>("NEVER");
      target.compareOp = compareOpMap[valueStr];
    }

    if (samplerNode["minLod"].IsScalar())
    {
      target.minLod = samplerNode["minLod"].as<float>(0.0f);
    }

    if (samplerNode["maxLod"].IsScalar())
    {
      target.maxLod = samplerNode["maxLod"].as<float>(VK_LOD_CLAMP_NONE);
    }

    if (samplerNode["borderColor"].IsScalar())
    {
      std::string valueStr =
          samplerNode["borderColor"].as<std::string>("FLOAT_TRANSPARENT_BLACK");
      target.borderColor = borderColorMap[valueStr];
    }

    if (samplerNode["unnormalizedCoordinates"].IsScalar())
    {
      target.unnormalizedCoordinates =
                        samplerNode["unnormalizedCoordinates"].as<bool>(false);
    }
  }

  //  Обрабатывает строки вида:
  //    inherits: filename
  //  Используется для обработки наследования настроек
  //  В ноде optionsNode ищет такую строку, и читает имена файлов, которые в ней
  //    описаны
  static std::vector<std::string> getInheritanceSources(YAML::Node optionsNode)
  {
    std::vector<std::string> fileNames;

    // Получаем список файлов, от которых необходимо унаследоваться
    YAML::Node inheritsNode = optionsNode["inherits"];
    if(inheritsNode.IsScalar())
    {
      fileNames.push_back(inheritsNode.as<std::string>("wrong value"));
    }
    else if(inheritsNode.IsSequence())
    {
      for(YAML::Node filenameNode : inheritsNode)
      {
        fileNames.push_back(filenameNode.as<std::string>("wrong value"));
      }
    }
    return fileNames;
  }
}