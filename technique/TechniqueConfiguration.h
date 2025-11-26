#pragma once

#include <array>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <technique/DescriptorSetType.h>
#include <technique/PassConfiguration.h>
#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/Sampler.h>

namespace mt
{
  //  Конфигурация техники(класс Technique).
  //  Набор предсобранных вариантов пайплайна и описаний дескриптор-сетов,
  //    ресурсов и юниформ переменных, которые в нем используются.
  //  Позволяет разделять один набор собранных пайплайнов между несколькими
  //    техниками (например, отрисовывать раздные данные данные одним и
  //    тем же способом, не пересобирая шейдеры для каждого набора данных).
  struct TechniqueConfiguration : public RefCounter
  {
    //  Дефайн в шейдере, который может принимать только ограниченное количество
    //  значений. Позволяет заранее скомпилировать все возможные вариации
    //  пайплайна и во время рендера просто выбирать нужный из списка.
    struct SelectionDefine
    {
      std::string name;
      std::vector<std::string> valueVariants;
      //  Веса селекшена для отдельных проходов
      //  weight используется для выбора пайплайна из набора скомпилированных
      //    PipelineIndex = index1 * weight1 + index2 * weight2 + ...
      //    Здесь indexX - это номер выбранного значения из valueVariants для
      //                    дефайна X
      //          weightX - значение weight для нужного прохода
      std::vector<uint32_t> weights;
    };
    std::vector<SelectionDefine> selections;

    ConstRef<PipelineLayout> pipelineLayout;
    std::vector<PassConfiguration> _passes;

    struct DescriptorSet
    {
      DescriptorSetType type;
      ConstRef<DescriptorSetLayout> layout;
    };
    std::vector<DescriptorSet> descriptorSets;

    //  Базовый тип, лежащий в основе типа параметра
    //  Например для float - это FLOAT_TYPE, для uvec3 - INT_TYPE,
    //    для vec4 - FLOAT_TYPE
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
      uint32_t offsetInBuffer;//  Смещение данных относительно начала буфера
      uint32_t size;          //  Размер занимаемых данных в байтах

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
      DescriptorSetType set;        //  К какому дескриптер сету должен быть
                                    //  привязан буфер
      uint32_t binding;             //  Индекс привязки к дескриптер сету
      std::string name;
      size_t size;                  //  Размер в байтах
      size_t volatileContextOffset; //  Смещение внутри буфера
                                    //  TechniqueVolatileContext

      std::vector<UniformVariable> variables;
    };
    std::vector<UniformBuffer> uniformBuffers;
    //  Суммарный размер всех юниформ буферов в статик сете
    size_t volatileUniformBuffersSize = 0;

    //  Описание отдельного источника данных, который используется пайплайном
    //  Здесь описываются текстуры, буферы, сэмплеры и т.д
    struct Resource
    {
      VkDescriptorType type;      //  Тип ресурса
      DescriptorSetType set;      //  К какому дескриптер сету должен быть
                                  //  привязан ресурс
      uint32_t bindingIndex;      //  Индекс привязки к дескриптер сету
      //  В каких шейдерах используется
      VkShaderStageFlags shaderStages;
      //  На каких стадиях используется (нужно для автоконтроля лэйаутов)
      VkPipelineStageFlags pipelineStages;
      std::string name;
      bool writeAccess;
      uint32_t count;             //  Если в один бинд прибинжен массив
                                  //  то здесь будет его размер. Иначе 1;
    };
    std::vector<Resource> resources;

    // Дефолтные сэмплеры. Настраиваются через конфигурацию (файл техники)
    struct DefaultSampler
    {
      std::string resourceName;
      ConstRef<Sampler> defaultSampler;
    };
    using Samplers = std::vector<DefaultSampler>;
    Samplers defaultSamplers;
  };
}