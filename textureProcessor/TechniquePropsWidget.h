#pragma once

#include <map>
#include <memory>

#include <TechniquePropertyWidget.h>
#include <TechniquePropsWidgetCommon.h>

namespace YAML
{
  class Emitter;
  class Node;
};

namespace mt
{
  class Technique;
};

class TechniquePropsWidget
{
public:
  explicit TechniquePropsWidget(mt::Technique& technique,
                                const TechniquePropsWidgetCommon& commonData);
  TechniquePropsWidget(const TechniquePropsWidget&) = delete;
  TechniquePropsWidget& operator = (const TechniquePropsWidget&) = delete;
  virtual ~TechniquePropsWidget() = default;

  inline mt::Technique& technique() const noexcept;

  void makeGUI();

  void save(YAML::Emitter& target) const;
  void load(const YAML::Node& source);

private:
  void _updateFromTechnique();
  void _addSubwidget(const std::string& fullName, const std::string& shortName);

private:
  mt::Technique& _technique;
  size_t _lastConfigurationRevision;

  TechniquePropsWidgetCommon _commonData;

  using Subwidgets =
                std::map<std::string, std::unique_ptr<TechniquePropertyWidget>>;
  Subwidgets _subwidgets;
};

inline mt::Technique& TechniquePropsWidget::technique() const noexcept
{
  return _technique;
}
