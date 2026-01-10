#include <stdexcept>
#include <vector>

#include <yaml-cpp/yaml.h>

#include <technique/TechniqueConfigurator.h>
#include <technique/TechniqueLoader.h>
#include <util/ContentLoader.h>
#include <util/vkMeta.h>
#include <vkr/image/ImageFormatFeatures.h>

namespace fs = std::filesystem;

namespace mt
{
  struct FrameBufferDescription
  {
    std::vector<VkFormat> colorTargets;
    VkFormat depthTarget = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  };


  //  Загрузить настройки техники из отдельной YAML ноды
  static void loadConfigurator( TechniqueConfigurator& target,
                                YAML::Node node,
                                std::unordered_set<fs::path>* usedFiles);
  // Загрузить список селекшенов для отдельного прохода
  static void loadSelectionsList(YAML::Node passNode, PassConfigurator& target);
  //  Загрузить список селекшенов конфигурации
  static TechniqueConfigurator::Selections loadSelections(
                                                      YAML::Node selectionsNode);
  //  Загрузить один отдельный проход
  static void loadPass( YAML::Node passNode,
                        PassConfigurator& target,
                        std::unordered_set<fs::path>* usedFiles);
  static void loadShaders(YAML::Node passNode, PassConfigurator& target);
  static void updateFrameBufferFormat(YAML::Node frameBufferNode,
                                      FrameBufferDescription& target,
                                      const std::string& passName,
                                      std::unordered_set<fs::path>* usedFiles);
  static void loadGraphicSettings(YAML::Node settingsNode,
                                  PassConfigurator& target,
                                  std::unordered_set<fs::path>* usedFiles);
  static void updateStencilOp(YAML::Node opNode,
                              VkStencilOpState& target,
                              std::unordered_set<fs::path>* usedFiles);
  static void loadBlending( YAML::Node blendingNode,
                            PassConfigurator& target,
                            std::unordered_set<fs::path>* usedFiles);
  static void loadBlending( YAML::Node blendingNode,
                            PassConfigurator& target,
                            uint32_t attachmentIndex);
  static VkBlendFactor getBlendFactor(YAML::Node settings, const char* name);
  static VkBlendOp getBlendOp(YAML::Node settings, const char* name);
  static void loadDefaultSamplers(YAML::Node techniqueNode,
                                  TechniqueConfigurator& target,
                                  std::unordered_set<fs::path>* usedFiles);
  static void loadGUIHints(YAML::Node techniqueNode,
                           TechniqueConfigurator& target);
  //  Загрузить настройки сэмплера, полная версия с наследованием
  static void updateSamplerSettings(YAML::Node samplerNode,
                                    SamplerDescription& target,
                                    std::unordered_set<fs::path>* usedFiles);
  //  Загрузить настройки сэмплера без ссылок на внешние файлы и наследования
  //  свойств
  void loadSamplerDescription(YAML::Node samplerNode,
                              SamplerDescription& target);
  static std::vector<fs::path> getInheritanceSources(YAML::Node optionsNode);

  YAML::Node readFile(const fs::path& file,
                      std::unordered_set<fs::path>* usedFiles)
  {
    if(usedFiles != nullptr)
    {
      usedFiles->insert(file.lexically_normal());
    }

    ContentLoader& loader = ContentLoader::getLoader();
    std::string fileText = loader.loadText(file);
    if(fileText.empty()) throw std::runtime_error(std::string((const char*)file.u8string().c_str()) + " : file is empty");
    YAML::Node config = YAML::Load(fileText);
    return config;
  }

  void loadConfigurator(TechniqueConfigurator& target, const fs::path& file)
  {
    loadConfigurator(target, file, nullptr);
  }

  void loadConfigurator(TechniqueConfigurator& target,
                        const fs::path& file,
                        std::unordered_set<fs::path>* usedFiles)
  {
    try
    {
      loadConfigurator(target, readFile(file, usedFiles), usedFiles);
    }
    catch (const std::runtime_error& error)
    {
      std::string filename = (const char*)file.u8string().c_str();
      throw std::runtime_error(filename + ": " + error.what());
    }
  }

  static void loadConfigurator( TechniqueConfigurator& target,
                                YAML::Node node,
                                std::unordered_set<fs::path>* usedFiles)
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
        loadPass(iPass->second, *newPass, usedFiles);
        target.addPass(std::move(newPass));
      }

      loadDefaultSamplers(node, target, usedFiles);
      loadGUIHints(node, target);
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

  static void loadPass( YAML::Node passNode,
                        PassConfigurator& target,
                        std::unordered_set<fs::path>* usedFiles)
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
                              target.name(),
                              usedFiles);
      if(frameBufferDescription.colorTargets.empty()) throw std::runtime_error(target.name() + ": unable to find 'color' targets in 'frameBuffer' description");
      FrameBufferFormat fbFormat( frameBufferDescription.colorTargets,
                                  frameBufferDescription.depthTarget,
                                  frameBufferDescription.samples);
      target.setFrameBufferFormat(&fbFormat);

      // Настройки фиксированных стадий пайплайна
      loadGraphicSettings(passNode["graphicSettings"], target, usedFiles);
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
      std::filesystem::path file = ((const char8_t*)shaderFile.c_str());
      shaders.push_back({ .stage = shaderStageMap[shaderStage],
                          .file = file});
    }

    target.setShaders(shaders);
  }

  // Загрузить настройки фрэйм буфера и объединить их с тем, что есть в target
  static void updateFrameBufferFormat(YAML::Node frameBufferNode,
                                      FrameBufferDescription& target,
                                      const std::string& passName,
                                      std::unordered_set<fs::path>* usedFiles)
  {
    if (!frameBufferNode.IsMap())  throw std::runtime_error(passName + ": unable to parse 'frameBuffer' description");

    // Для начала смотрим, надо ли наследовать настройки от кого-нибудь
    std::vector<fs::path> parents = getInheritanceSources(frameBufferNode);
    for(const fs::path& file : parents)
    {
      try
      {
        YAML::Node config = readFile(file, usedFiles);
        updateFrameBufferFormat(config, target, passName, usedFiles);
      }
      catch(std::runtime_error error)
      {
        std::string filename = (const char*)file.u8string().c_str();
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
                                  PassConfigurator& target,
                                  std::unordered_set<fs::path>* usedFiles)
  {
    if (!settingsNode.IsMap()) return;

    // Для начала прогружаем унаследованные настройки
    std::vector<fs::path> parents = getInheritanceSources(settingsNode);
    for (const fs::path& file : parents)
    {
      try
      {
        YAML::Node config = readFile(file, usedFiles);
        loadGraphicSettings(config, target, usedFiles);
      }
      catch (std::runtime_error error)
      {
        std::string filename = (const char*)file.u8string().c_str();
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
      updateStencilOp(settingsNode["frontStencilOp"],
                      stencilSettings,
                      usedFiles);
      target.setFrontStencilOp(stencilSettings);
    }

    if (settingsNode["backStencilOp"].IsMap())
    {
      VkStencilOpState stencilSettings;
      updateStencilOp(settingsNode["backStencilOp"],
                      stencilSettings,
                      usedFiles);
      target.setBackStencilOp(stencilSettings);
    }

    loadBlending(settingsNode["blending"], target, usedFiles);

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

  static void updateStencilOp(YAML::Node opNode,
                              VkStencilOpState& target,
                              std::unordered_set<fs::path>* usedFiles)
  {
    if(!opNode.IsMap()) return;

    // Для начала прогружаем унаследованные настройки
    std::vector<fs::path> parents = getInheritanceSources(opNode);
    for (const fs::path& file : parents)
    {
      try
      {
        YAML::Node config = readFile(file, usedFiles);
        updateStencilOp(config, target, usedFiles);
      }
      catch (std::runtime_error error)
      {
        std::string filename = (const char*)file.u8string().c_str();
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

  static void loadBlending( YAML::Node blendingNode,
                            PassConfigurator& target,
                            std::unordered_set<fs::path>* usedFiles)
  {
    // Для начала прогружаем унаследованные настройки
    std::vector<fs::path> parents = getInheritanceSources(blendingNode);
    for (const fs::path& file : parents)
    {
      try
      {
        YAML::Node config = readFile(file, usedFiles);
        loadBlending(config, target, usedFiles);
      }
      catch (std::runtime_error error)
      {
        std::string filename = (const char*)file.u8string().c_str();
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
                                  TechniqueConfigurator& target,
                                  std::unordered_set<fs::path>* usedFiles)
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

      SamplerDescription samplerDescription;
      updateSamplerSettings(iSampler->second, samplerDescription, usedFiles);

      sampler.defaultSampler = ConstRef(new Sampler(target.device(),
                                                    samplerDescription));
      samplers.push_back(sampler);
    }

    target.setDefaultSamplers(samplers);
  }

  static void updateSamplerSettings(YAML::Node samplerNode,
                                    SamplerDescription& target,
                                    std::unordered_set<fs::path>* usedFiles)
  {
    // Для начала прогружаем унаследованные настройки
    std::vector<fs::path> parents = getInheritanceSources(samplerNode);
    for (const fs::path& file : parents)
    {
      try
      {
        YAML::Node config = readFile(file, usedFiles);
        updateSamplerSettings(config, target, usedFiles);
      }
      catch (std::runtime_error error)
      {
        std::string filename = (const char*)file.u8string().c_str();
        throw std::runtime_error(filename + ": " + error.what());
      }
    }

    // Дальше грузим собственные настройки
    loadSamplerDescription(samplerNode, target);
  }

  //  Загрузить настройки сэмплера без ссылок на внешние файлы и наследования
  //  свойств
  void loadSamplerDescription(YAML::Node samplerNode, SamplerDescription& target)
  {
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

  SamplerDescription loadSamplerDescription(const YAML::Node& samplerNode)
  {
    SamplerDescription result;
    loadSamplerDescription(samplerNode, result);
    return result;
  }

  //  Обрабатывает строки вида:
  //    inherits: filename
  //  Используется для обработки наследования настроек
  //  В ноде optionsNode ищет такую строку, и читает имена файлов, которые в ней
  //    описаны
  static std::vector<fs::path> getInheritanceSources(YAML::Node optionsNode)
  {
    std::vector<fs::path> files;

    // Получаем список файлов, от которых необходимо унаследоваться
    YAML::Node inheritsNode = optionsNode["inherits"];
    if(inheritsNode.IsScalar())
    {
      std::string filename = inheritsNode.as<std::string>("wrong value");
      fs::path filePatch((const char8_t*)filename.c_str());
      files.push_back(filePatch);
    }
    else if(inheritsNode.IsSequence())
    {
      for(YAML::Node filenameNode : inheritsNode)
      {
        std::string filename = filenameNode.as<std::string>("wrong value");
        fs::path filePatch((const char8_t*)filename.c_str());
        files.push_back(filePatch);
      }
    }
    return files;
  }

  void loadGUIHints(YAML::Node techniqueNode, TechniqueConfigurator& target)
  {
    YAML::Node listNode = techniqueNode["GUIHints"];
    if(!listNode.IsMap()) return;

    for(YAML::const_iterator iHint = listNode.begin();
        iHint != listNode.end();
        iHint++)
    {
      // Имя элемента
      TechniqueConfigurator::GUIHint hint;
      hint.elementName = iHint->first.as<std::string>("");
      if(hint.elementName.empty()) continue;

      // Грузим хинты, которые применяются к элементу
      YAML::Node valuesNode = iHint->second;
      if (valuesNode.IsSequence())
      {
        for (YAML::Node value : valuesNode)
        {
          std::string valueStr = value.as<std::string>("");
          if(valueStr == "hidden")
          {
            hint.hints = hint.hints | TechniqueConfiguration::GUI_HINT_HIDDEN;
          }
        }
      }

      target.addGUIHint(hint);
    }
  }
}