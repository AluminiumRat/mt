#pragma once

#include <string>
#include <vector>

#include <technique/Technique.h>
#include <util/Ref.h>
#include <util/RefCounter.h>

namespace mt
{
  class CommandProducerGraphic;
  class DrawCommandList;
  class FrameContext;

  //  Общие данные для MeshDrawable
  //  Позволяет переиспользовать данные для отрисовки одинаковых мешей
  //    в разном контексте(с разными настройками) и использовать инстансинг
  class MeshAsset : public RefCounter
  {
  public:
    //  Конфигурация отдельного прохода отрисовки меша
    //  Используется во время настройки ассета
    struct PassConfig
    {
      std::string frameTypeName;
      std::string stageName;
      int32_t layer;
      std::string passName;
    };

    //  Полная конфигурация для всего меш-ассета
    //  Используется во время загрузки/настройки ассета
    struct Configuration
    {
      Ref<Technique> technique;
      std::vector<PassConfig> passes;
      uint32_t vertexCount;
      uint32_t maxInstancesCount;
    };

    //  Набор доступных стадий для одного отдельного фрэйм-типа
    //  Используется при отрисовке мешей
    using StageIndices = std::vector<uint32_t>;

    //  Информация об отдельном проходе отрисовки меша
    //  Используется при отрисовке мешей
    struct PassInfo
    {
      int32_t layer;
      TechniquePass* pass;
      uint32_t commandGroup;
    };

    //  Информация обо всех проходах отрисовки для одной отдельной стадии
    //    одного отдельного фрэйм-типа
    //  Используется при отрисовке мешей
    using StagePasses = std::vector<PassInfo>;

  public:
    explicit MeshAsset(const char* debugName);
    MeshAsset(const MeshAsset&) = delete;
    MeshAsset& operator = (const MeshAsset&) = delete;
  protected:
    virtual ~MeshAsset() = default;

  public:
    inline const std::string& debugName() const noexcept;

    virtual void setConfiguration(const Configuration& configuration);

    inline Technique* technique() const noexcept;
    inline uint32_t vertexCount() const noexcept;
    inline uint32_t maxInstancesCount() const noexcept;

    //  Какие стадии поддерживаются для отдельного фрэйм-типа
    inline const StageIndices& availableStages(
                                        uint32_t frameTypeIndex) const noexcept;

    //  Какие есть проходы для отдельной стадии отдельного фрэйм-типа
    inline const StagePasses& passes( uint32_t frameTypeIndex,
                                      uint32_t stageIndex) const noexcept;

  private:
    void _clearConfiguration() noexcept;
    void _addPass(uint32_t frameTypeIndex,
                  uint32_t stageIndex,
                  int32_t layer,
                  TechniquePass& pass);
    void _addAvailableStage(uint32_t frameTypeIndex, uint32_t stageIndex);

  private:
    std::string _debugName;

    Ref<Technique> _technique;
    uint32_t _vertexCount;
    uint32_t _maxInstancesCount;

    //  Стадии, доступные для отдельных фрэйм-типов. Индекс в векторе - это
    //  frameTypeIndex(HLDLib::getFrameTypeIndex)
    using AvailableStages = std::vector<StageIndices>;
    AvailableStages _availableStages;

    //  Информация по отрисовке на всех стадиях отдельного фрэйм-типа
    //  Индекс в векторе - это индекс стадии (HLDLib::getStageIndex)
    using FrameStages = std::vector<StagePasses>;
    //  Информация по отрисовке на всех стадиях всех фрэйм-типов
    //  Индекс вектора - это frameTypeIndex(HLDLib::getFrameTypeIndex)
    using DrawMap = std::vector<FrameStages>;
    DrawMap _drawMap;

    //  Пустой вектор. Ссылка на него возвращается из метода availableStages,
    //  когда нечего вернуть из _availableStages
    StageIndices _emptyFrameInfo;
    //  Пустой вектор. Ссылка на него возвращается из метода passes, когда
    //  нечего вернуть из _drawMap
    StagePasses _emptyStageInfo;
  };

  inline const std::string& MeshAsset::debugName() const noexcept
  {
    return _debugName;
  }

  inline Technique* MeshAsset::technique() const noexcept
  {
    return _technique.get();
  }

  inline uint32_t MeshAsset::vertexCount() const noexcept
  {
    return _vertexCount;
  }

  inline uint32_t MeshAsset::maxInstancesCount() const noexcept
  {
    return _maxInstancesCount;
  }

  inline const MeshAsset::StageIndices& MeshAsset::availableStages(
                                        uint32_t frameTypeIndex) const noexcept
  {
    if(frameTypeIndex >= _availableStages.size()) return _emptyFrameInfo;
    return _availableStages[frameTypeIndex];
  }

  inline const MeshAsset::StagePasses&
                          MeshAsset::passes(uint32_t frameTypeIndex,
                                            uint32_t stageIndex) const noexcept
  {
    if (frameTypeIndex >= _drawMap.size()) return _emptyStageInfo;
    const FrameStages& frameStages = _drawMap[frameTypeIndex];
    if (stageIndex >= frameStages.size()) return _emptyStageInfo;
    return frameStages[stageIndex];
  }
}