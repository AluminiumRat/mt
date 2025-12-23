#pragma once

#include <map>
#include <memory>

#include <TechniquePropertyWidget.h>

namespace mt
{
  class Technique;
};

class TechniquePropsWidget
{
public:
  explicit TechniquePropsWidget(mt::Technique& technique);
  TechniquePropsWidget(const TechniquePropsWidget&) = delete;
  TechniquePropsWidget& operator = (const TechniquePropsWidget&) = delete;
  virtual ~TechniquePropsWidget() = default;

  inline mt::Technique& technique() const noexcept;

  void makeGUI();

private:
  void _updateFromTechnique();
  void _addSubwidget(const std::string& fullName, const std::string& shortName);

private:
  mt::Technique& _technique;
  size_t _lastConfigurationRevision;

  using Subwidgets =
                std::map<std::string, std::unique_ptr<TechniquePropertyWidget>>;
  Subwidgets _subwidgets;
};

inline mt::Technique& TechniquePropsWidget::technique() const noexcept
{
  return _technique;
}
