#include <stdexcept>

#include <technique/TechniqueConfigurator.h>
#include <technique/TechniqueLoader.h>
#include <util/vkMeta.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/pipeline/ShaderLoader.h>

namespace mt
{
  // Загрузить список селекшенов для отдельного прохода
  void loadSelectionsList(YAML::Node passNode, PassConfigurator& target);
  void loadShaders(YAML::Node passNode, PassConfigurator& target);
  void loadFrameBufferFormat(YAML::Node passNode, PassConfigurator& target);
  void loadGraphicSettings(YAML::Node passNode, PassConfigurator& target);
  VkStencilOpState loadStencilOp(YAML::Node opNode);
  void loadBlending(YAML::Node settings, PassConfigurator& target);
  void loadBlending(YAML::Node blendingNode,
                    PassConfigurator& target,
                    uint32_t attachmentIndex);
  VkBlendFactor getBlendFactor(YAML::Node settings, const char* name);
  VkBlendOp getBlendOp(YAML::Node settings, const char* name);

  void loadConfigurator(TechniqueConfigurator& target, const char* filename)
  {
    ShaderLoader& textLoader = ShaderLoader::getShaderLoader();
    std::string techniqueText = textLoader.loadText(filename);
    try
    {
      YAML::Node config = YAML::Load(techniqueText);
      loadConfigurator(target, config);
    }
    catch (const std::runtime_error& error)
    {
      throw std::runtime_error(std::string(filename) + ": " + error.what());
    }
  }

  void loadConfigurator(TechniqueConfigurator& target, YAML::Node node)
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
    }
    catch(...)
    {
      target.clear();
      throw;
    }
  }

  TechniqueConfigurator::Selections loadSelections(YAML::Node selectionsNode)
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

  void loadPass(YAML::Node passNode, PassConfigurator& target)
  {
    if(!passNode.IsMap()) throw std::runtime_error(target.name() + ": unable to parse pass configuration");

    std::string pipeline = passNode["pipeline"].as<std::string>("GRAPHIC");
    target.setPipelineType(pipelineMap[pipeline]);

    loadSelectionsList(passNode, target);
    loadShaders(passNode, target);
    loadFrameBufferFormat(passNode, target);
    loadGraphicSettings(passNode, target);
  }

  void loadSelectionsList(YAML::Node passNode, PassConfigurator& target)
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

  void loadShaders(YAML::Node passNode, PassConfigurator& target)
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

  void loadFrameBufferFormat(YAML::Node passNode, PassConfigurator& target)
  {
    if(target.pipelineType() != AbstractPipeline::GRAPHIC_PIPELINE) return;

    YAML::Node fbNode = passNode["frameBuffer"];
    if (!fbNode.IsMap())  throw std::runtime_error(target.name() + ": unable to parse 'frameBuffer' description");

    // Парсим цветовые таргеты
    std::vector<VkFormat> colorTargets;
    YAML::Node colorNode = fbNode["color"];
    // color может быть как отдельным форматом, так и массивом форматов
    if(colorNode.IsScalar())
    {
      std::string formatName = colorNode.as<std::string>();
      colorTargets.push_back(getFormatFeatures(formatName).format);
    }
    else if(colorNode.IsSequence())
    {
      if(colorNode.size() > FrameBufferFormat::maxColorAttachments) throw std::runtime_error(target.name() + ": frameBuffer: too many targets in 'color' array");
      for(YAML::Node formatNode : colorNode)
      {
        std::string formatName = formatNode.as<std::string>();
        colorTargets.push_back(getFormatFeatures(formatName).format);
      }
    }
    else throw std::runtime_error(target.name() + ": unable to parse 'color' targets in 'frameBuffer' description");

    // Парсим деф-стенсил
    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    YAML::Node depthStencilNode = fbNode["depthStencil"];
    if(depthStencilNode.IsScalar())
    {
      std::string formatName = depthStencilNode.as<std::string>();
      depthStencilFormat = getFormatFeatures(formatName).format;
    }

    // Мультисэмплинг
    std::string samplesStr = passNode["samples"].as<std::string>("1");
    VkSampleCountFlagBits samples = sampleCountMap[samplesStr];

    FrameBufferFormat fbFormat(colorTargets, depthStencilFormat, samples);
    target.setFrameBufferFormat(&fbFormat);
  }

  void loadGraphicSettings(YAML::Node passNode, PassConfigurator& target)
  {
    if (target.pipelineType() != AbstractPipeline::GRAPHIC_PIPELINE) return;
    YAML::Node settings = passNode["graphicSettings"];
    if (!settings.IsMap()) return;

    std::string valueStr = settings["topology"].as<std::string>("TRIANGLE_LIST");
    target.setTopology(topologyMap[valueStr]);

    valueStr = settings["polygonMode"].as<std::string>("FILL");
    target.setPolygonMode(polygonModeMap[valueStr]);

    target.setLineWidth(settings["lineWidth"].as<float>(1.0f));

    valueStr = settings["cullMode"].as<std::string>("NONE");
    target.setCullMode(cullModeMap[valueStr]);

    valueStr = settings["frontFace"].as<std::string>("COUNTER_CLOCKWISE");
    target.setFrontFace(fronFaceMap[valueStr]);

    target.setRasterizationDiscardEnable(
                        settings["rasterizationDiscardEnable"].as<bool>(false));

    target.setDepthTestEnable(settings["depthTest"].as<bool>(false));
    target.setDepthWriteEnable(settings["depthWrite"].as<bool>(false));

    valueStr = settings["depthCompareOp"].as<std::string>("NEVER");
    target.setDepthCompareOp(compareOpMap[valueStr]);

    target.setDepthClampEnable(settings["depthClampEnable"].as<bool>(false));
    target.setDepthBiasEnable(settings["depthBias"].as<bool>(false));
    target.setDepthBiasConstantFactor(
                                settings["depthBiasConstant"].as<float>(0.0f));
    target.setDepthBiasSlopeFactor(settings["depthBiasSlope"].as<float>(0.0f));
    target.setDepthBiasClamp(settings["depthBiasClamp"].as<bool>(false));
    target.setDepthBoundsTestEnable(
                                  settings["depthBoundsTest"].as<bool>(false));
    target.setMinDepthBounds(settings["minDepthBounds"].as<float>(0.0f));
    target.setMaxDepthBounds(settings["maxDepthBounds"].as<float>(0.0f));

    target.setFrontStencilOp(loadStencilOp(settings["frontStencilOp"]));
    target.setBackStencilOp(loadStencilOp(settings["backStencilOp"]));

    loadBlending(settings, target);
  }

  VkStencilOpState loadStencilOp(YAML::Node opNode)
  {
    VkStencilOpState result{};
    if(!opNode.IsMap()) return result;

    std::string valueStr = opNode["failOp"].as<std::string>("KEEP");
    result.failOp = stencilOpMap[valueStr];

    valueStr = opNode["passOp"].as<std::string>("KEEP");
    result.passOp = stencilOpMap[valueStr];

    valueStr = opNode["depthFailOp"].as<std::string>("KEEP");
    result.depthFailOp = stencilOpMap[valueStr];

    valueStr = opNode["compareOp"].as<std::string>("NEVER");
    result.compareOp = compareOpMap[valueStr];

    result.compareMask = opNode["compareMask"].as<uint32_t>(0);
    result.writeMask = opNode["writeMask"].as<uint32_t>(0);
    result.reference = opNode["reference"].as<uint32_t>(0);

    return result;
  }

  void loadBlending(YAML::Node settings, PassConfigurator& target)
  {
    // Загружаем блэндинг отдельных таргетов
    YAML::Node blendingNode = settings["blending"];
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

    // Логические операции
    YAML::Node logicOp = settings["blendLogicOp"];
    if(logicOp.IsScalar())
    {
      target.setBlendLogicOp(logicOpMap[logicOp.as<std::string>("")]);
    }

    // Константы блэндинга
    YAML::Node constants = settings["blendConstants"];
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

  void loadBlending(YAML::Node blendingNode,
                    PassConfigurator& target,
                    uint32_t attachmentIndex)
  {
    if(!blendingNode.IsMap()) throw std::runtime_error(target.name() + ": unable to parse 'blending' description");

    target.setBlendEnable(attachmentIndex,
                          blendingNode["blendEnable"].as<bool>(false));
    // Блендинг цветов
    target.setSrcColorBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "srcColorBlendFactor"));
    target.setDstColorBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "dstColorBlendFactor"));
    target.setColorBlendOp( attachmentIndex,
                            getBlendOp(blendingNode, "colorBlendOp"));
    // Блендинг альфы
    target.setSrcAlphaBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "srcAlphaBlendFactor"));
    target.setDstAlphaBlendFactor(
                          attachmentIndex,
                          getBlendFactor(blendingNode, "dstAlphaBlendFactor"));
    target.setAlphaBlendOp( attachmentIndex,
                            getBlendOp(blendingNode, "alphaBlendOp"));
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

  VkBlendFactor getBlendFactor(YAML::Node settings, const char* name)
  {
    std::string valueStr = settings[name].as<std::string>("ZERO");
    return blendFactorMap[valueStr];
  }

  VkBlendOp getBlendOp(YAML::Node settings, const char* name)
  {
    std::string valueStr = settings[name].as<std::string>("ADD");
    return blendOpMap[valueStr];
  }
}