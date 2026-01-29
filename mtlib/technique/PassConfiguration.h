#pragma once

#include <string>
#include <vector>

#include <util/Ref.h>
#include <vkr/pipeline/AbstractPipeline.h>

namespace mt
{
  //  Конфигурация для отдельного прохода техники(класс Technique)
  //  Входит в состав TechniqueConfiguration
  //  Набор предсобранных вариантов пайплайна и доп. информации к нему
  struct PassConfiguration
  {
    std::string name;

    AbstractPipeline::Type pipelineType = AbstractPipeline::GRAPHIC_PIPELINE;

    //  Дополнительные данные для автонастройки Drawable-ов в библиотеке hld
    std::string frameType;
    std::string stageName;
    int32_t layer = 0;
    uint32_t maxInstances = 1;

    //  Различные варианты собранных пайплайнов для разных значений селекшенов
    //  Если pipelineType = GRAPHIC_PIPELINE, то все пайплайны, которые тут
    //    находятся - это mt::GraphicPipeline. Если pipelineType =
    //    COMPUTE_PIPELINE, то все пайплайны - это mt::ComputePipeline
    std::vector<ConstRef<AbstractPipeline>> pipelineVariants;

    PassConfiguration() noexcept = default;
    PassConfiguration(const PassConfiguration&) = default;
    PassConfiguration(PassConfiguration&&) noexcept = default;
    PassConfiguration& operator = (const PassConfiguration&) = default;
    PassConfiguration& operator = (PassConfiguration&&) noexcept = default;
    ~PassConfiguration() noexcept = default;
  };
}