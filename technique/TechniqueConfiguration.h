#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <technique/DescriptorSetType.h>
#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/FrameBufferFormat.h>

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
      //  weight используется для выбора пайплайна из набора скомпилированных
      //    PipelineIndex = index1 * weight1 + index2 * weight2 + ...
      //    Здесь indexX - это номер выбранного значения из valueVariants для
      //                    дефайна X
      //          weightX - значение weight для этого дефайна
      uint32_t weight;
    };
    std::vector<SelectionDefine> selections;

    AbstractPipeline::Type pipelineType;
    ConstRef<PipelineLayout> pipelineLayout;

    std::vector<ConstRef<GraphicPipeline>> graphicPipelineVariants;

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
  };
}