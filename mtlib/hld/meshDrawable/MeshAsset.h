#pragma once

#include <memory>
#include <span>
#include <vector>

#include <hld/drawCommand/DrawCommand.h>
#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>
#include <technique/Technique.h>
#include <util/AABB.h>
#include <util/Assert.h>
#include <util/Ref.h>
#include <util/RefCounter.h>
#include <util/Signal.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  //  Разделяемые данные для MeshDrawable
  //  Позволяет переиспользовать данные для отрисовки одинаковых мешей
  //    в разном контексте(с разными настройками) и использовать инстансинг
  class MeshAsset : public RefCounter
  {
  public:
    //  Набор доступных стадий для одного отдельного фрэйм-типа
    using StageIndices = std::vector<StageIndex>;

    //  Информация об отдельном проходе отрисовки меша
    struct PassInfo
    {
      int32_t layer;
      DrawCommand::Group commandGroup;

      Technique* technique;
      TechniquePass* pass;

      const UniformVariable* positionMatrix;
      const UniformVariable* prevPositionMatrix;
      const UniformVariable* bivecMatrix;
    };

    //  Информация обо всех проходах отрисовки для одной отдельной стадии
    //    одного отдельного фрэйм-типа
    using StagePasses = std::vector<PassInfo>;

  public:
    explicit MeshAsset(const char* debugName);
    MeshAsset(const MeshAsset&) = delete;
    MeshAsset& operator = (const MeshAsset&) = delete;
  protected:
    virtual ~MeshAsset();

  public:
    inline const std::string& debugName() const noexcept;

    //  Какие стадии поддерживаются для отдельного фрэйм-типа
    inline const StageIndices& availableStages(
                                  FrameTypeIndex frameTypeIndex) const noexcept;

    //  Какие есть проходы для отдельной стадии отдельного фрэйм-типа
    inline const StagePasses& passes( FrameTypeIndex frameTypeIndex,
                                      StageIndex stageIndex) const noexcept;

    //  Удалитть все техники и буферы, инвалидировать AABB
    void clear() noexcept;

    //  Добавить технику рисования
    void addTechnique(std::unique_ptr<Technique> technique);
    //  Добавить сразу несколько техник.
    void addTechniques(std::span<std::unique_ptr<Technique>> techniques);
    void deleteTechnique(const Technique& technique) noexcept;
    inline size_t techniquesCount() const noexcept;
    inline const Technique& technique(size_t index) const noexcept;
    inline Technique& technique(size_t index) noexcept;

    //  Установить буфер, который будет использоваться во всех техниках.
    //  Как правило это один или несколько буферов вершин, но никто не
    //  запрещает закидывать туда какие угодно статические данные
    //  bufferName - имя, под которым буфер используется в шейдерах
    void setCommonBuffer(const char* bufferName, const DataBuffer& buffer);

    //  Количество точек на отрисовку
    inline uint32_t vertexCount() const noexcept;
    inline void setVertexCount(uint32_t newValue);

    //  Какое максимальное количество инстансов поддерживают техники отрисовки
    inline uint32_t maxInstancesCount() const noexcept;
    inline void setMaxInstancesCount(uint32_t newValue);

    inline const AABB& bound() const noexcept;
    inline void setBound(const AABB& newValue);

    //  Возвращает true, если хотябы одна из техник использует bivec матрицу
    //  (матрица поворота нормалей, обратная транспонированная от матрицы
    //  трансформации)
    inline bool usesBivecMatrix() const noexcept;

    //  Возвращает true, если хотябы одна из техник использует матрицу положения
    //  предыдущего кадра
    inline bool usesPrevMatrix() const noexcept;

  public:
    //  Вызывается, когда изменяется количество техник (clear, addTechnique и
    //    deleteTechnique) или какая-либо из техник меняет свою конфигурацию
    mutable Signal<> techniquesChanged;
    //  Вызывается, когда меняется AABB (clear или setBound)
    mutable Signal<> boundChanged;

  private:
    void _rebuildDrawMap();
    void _updateDrawmapFromTechnique(Technique& technique);
    void _addAvailableStage(FrameTypeIndex frameTypeIndex,
                            StageIndex stageIndex);
  private:
    struct BufferInfo
    {
      std::string name;
      ConstRef<DataBuffer> buffer;
    };
    using CommonBuffers = std::vector<BufferInfo>;

  private:
    std::string _debugName;

    using Techniques = std::vector<std::unique_ptr<Technique>>;
    Techniques _techniques;

    Slot<> _onTechniqueUpdatedSlot;

    //  Стадии, доступные для отдельных фрэйм-типов. Индекс в векторе - это
    //  frameTypeIndex(HLDLib::getFrameTypeIndex)
    using AvailableStages = std::vector<StageIndices>;
    AvailableStages _availableStages;
    //  Пустой вектор. Ссылка на него возвращается из метода availableStages,
    //  когда нечего вернуть из _availableStages
    StageIndices _emptyFrameInfo;

    //  Информация по отрисовке на всех стадиях отдельного фрэйм-типа
    //  Индекс в векторе - это индекс стадии (HLDLib::getStageIndex)
    using FrameStages = std::vector<StagePasses>;
    //  Информация по отрисовке на всех стадиях всех фрэйм-типов
    //  Индекс вектора - это frameTypeIndex(HLDLib::getFrameTypeIndex)
    using DrawMap = std::vector<FrameStages>;
    DrawMap _drawMap;
    //  Пустой вектор. Ссылка на него возвращается из метода passes, когда
    //  нечего вернуть из _drawMap
    StagePasses _emptyStageInfo;

    bool _usesBivecMatrix;
    bool _usesPrevMatrix;

    CommonBuffers _commonBuffers;

    uint32_t _vertexCount;
    uint32_t _maxInstancesCount;

    AABB _bound;
  };

  inline const std::string& MeshAsset::debugName() const noexcept
  {
    return _debugName;
  }

  inline const MeshAsset::StageIndices& MeshAsset::availableStages(
                                  FrameTypeIndex frameTypeIndex) const noexcept
  {
    if (frameTypeIndex >= _availableStages.size()) return _emptyFrameInfo;
    return _availableStages[frameTypeIndex];
  }

  inline const MeshAsset::StagePasses&
                                MeshAsset::passes(
                                          FrameTypeIndex frameTypeIndex,
                                          StageIndex stageIndex) const noexcept
  {
    if (frameTypeIndex >= _drawMap.size()) return _emptyStageInfo;
    const FrameStages& frameStages = _drawMap[frameTypeIndex];
    if (stageIndex >= frameStages.size()) return _emptyStageInfo;
    return frameStages[stageIndex];
  }

  inline size_t MeshAsset::techniquesCount() const noexcept
  {
    return _techniques.size();
  }

  inline const Technique& MeshAsset::technique(size_t index) const noexcept
  {
    return *_techniques[index];
  }

  inline Technique& MeshAsset::technique(size_t index) noexcept
  {
    return *_techniques[index];
  }

  inline uint32_t MeshAsset::vertexCount() const noexcept
  {
    return _vertexCount;
  }

  inline void MeshAsset::setVertexCount(uint32_t newValue)
  {
    _vertexCount = newValue;
  }

  inline uint32_t MeshAsset::maxInstancesCount() const noexcept
  {
    return _maxInstancesCount;
  }

  inline void MeshAsset::setMaxInstancesCount(uint32_t newValue)
  {
    MT_ASSERT(newValue != 0);
    if(_maxInstancesCount == newValue) return;
    _maxInstancesCount = newValue;
    _rebuildDrawMap();
  }

  inline const AABB& MeshAsset::bound() const noexcept
  {
    return _bound;
  }

  inline void MeshAsset::setBound(const AABB& newValue)
  {
    if(_bound == newValue) return;
    _bound = newValue;
    boundChanged.emit();
  }

  inline bool MeshAsset::usesBivecMatrix() const noexcept
  {
    return _usesBivecMatrix;
  }

  inline bool MeshAsset::usesPrevMatrix() const noexcept
  {
    return _usesPrevMatrix;
  }
}