#pragma once

#include <map>
#include <memory>

#include <techniquePropertyGrid/TechniquePropertyWidget.h>
#include <techniquePropertyGrid/TechniquePropertyGridCommon.h>

namespace YAML
{
  class Emitter;
  class Node;
};

namespace mt
{
  class Technique;

  class TechniquePropertyGrid
  {
  public:
    explicit TechniquePropertyGrid(
                                mt::Technique& technique,
                                const TechniquePropertyGridCommon& commonData);
    TechniquePropertyGrid(const TechniquePropertyGrid&) = delete;
    TechniquePropertyGrid& operator = (const TechniquePropertyGrid&) = delete;
    virtual ~TechniquePropertyGrid() = default;

    inline mt::Technique& technique() const noexcept;

    void makeGUI();

    void save(YAML::Emitter& target) const;
    void load(const YAML::Node& source);

  private:
    void _updateFromTechnique();
    void _addSubwidget( const std::string& fullName,
                        const std::string& shortName);

  private:
    mt::Technique& _technique;
    size_t _lastConfigurationRevision;

    TechniquePropertyGridCommon _commonData;

    using Subwidgets =
                std::map<std::string, std::unique_ptr<TechniquePropertyWidget>>;
    Subwidgets _subwidgets;
  };

  inline mt::Technique& TechniquePropertyGrid::technique() const noexcept
  {
    return _technique;
  }
}