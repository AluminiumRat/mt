#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <technique/DescriptorSetType.h>
#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/FrameBufferFormat.h>

namespace mt
{
  //  Конфигурация техники(класс Technique) на какой-то определенной ревизии.
  //  Техника может создавать новые конфигурации, но объекты, которые привязаны
  //  к определенному состоянию, не должны его терять
  struct TechniqueConfiguration : public RefCounter
  {
    //  Может ли техника быть использована для построения пайплайна
    //  Может быть false, если, например, не удалось загрузить шейдерные модули
    bool isValid;

    ConstRef<PipelineLayout> pipelineLayout;

    struct Shader
    {
      std::unique_ptr<ShaderModule> shaderModule;
      VkShaderStageFlagBits stage;
    };
    std::vector<Shader> shaders;

    struct DescriptorSet
    {
      DescriptorSetType type;
      ConstRef<DescriptorSetLayout> layout;
    };
    std::vector<DescriptorSet> descriptorSets;

    //  Базовый тип, лежащий в основе типа параметра
    //  Например для float - это FLOAT_TYPE, для uvec3 - INT_TYPE,
    //    для vec4 - float
    enum BaseScalarType
    {
      BOOL_TYPE,
      INT_TYPE,
      FLOAT_TYPE,
      UNKNOWN_TYPE    //  Базовый тип не найден (например для массива структур)
                      //  параметр необходимо интерпретировать просто как
                      //  последовательность байт в памяти
    };

    // Описание отдельной юниформ переменной
    struct UniformVariable
    {
      std::string shortName;  //  Имя без названий родительских объектов, в
                              //    которых находится переменная
      std::string fullName;   //  Полное уникальное имя со всеми названиями
                              //    родительских объектов
      size_t offsetInBuffer;  //  Смещение данных относительно начала буфера
      size_t size;            //  Размер занимаемых данных в байтах

      BaseScalarType baseType;

      bool isVector;          //  не может быть true одновременно с isMatrix
      uint32_t vectorSize;

      bool isMatrix;          //  не может быть true одновременно с isVector
      glm::uvec2 matrixSize;  //  x - количество столбцов, y - строк

      bool isArray;           //  isArray может быть выставлен совместно с
                              //    isVector или isMatrix. В этом случае
                              //    параметр - это массив векторов или матриц
      uint32_t arraySize;
      uint32_t arrayStride;
    };

    struct UniformBuffer
    {
      DescriptorSetType set;      //  К какому дескриптер сету должен быть
                                  //  привязан буфер
      uint32_t binding;           //  Индекс привязки к дескриптер сету
      std::string name;
      VkShaderStageFlags stages;  //  В каких шейдерах используется
      size_t size;                //  Размер в байтах

      std::vector<UniformVariable> variables;
    };
    std::vector<UniformBuffer> uniformBuffers;

    //  Описание отдельного источника данных, который используется пайплайном
    //  Здесь описываются текстуры, буферы, сэмплеры и т.д
    struct Resource
    {
      VkDescriptorType type;      //  Тип ресурса
      DescriptorSetType set;      //  К какому дескриптер сету должен быть
                                  //  привязан ресурс
      uint32_t bindingIndex;      //  Индекс привязки к дескриптер сету
      VkShaderStageFlags stages;  //  В каких шейдерах используется
      std::string name;
      bool writeAccess;
      uint32_t count;             //  Если в один бинд прибинжен массив
                                  //  то здесь будет его размер. Иначе 1;
    };
    std::vector<Resource> resources;

    // Настройки графического пайплайна. Игнорируются компьют пайплайном.
    VkPrimitiveTopology topology;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendStateCreateInfo blendingState;
    std::array< VkPipelineColorBlendAttachmentState,
                FrameBufferFormat::maxColorAttachments> attachmentsBlending;
  };
}