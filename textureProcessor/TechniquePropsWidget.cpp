#include <Technique/Technique.h>

#include <imgui.h>

#include <yaml-cpp/yaml.h>

#include <gui/ImGuiPropertyGrid.h>

#include <TechniquePropsWidget.h>

TechniquePropsWidget::TechniquePropsWidget(
                                mt::Technique& technique,
                                const TechniquePropsWidgetCommon& commonData) :
  _technique(technique),
  _lastConfigurationRevision(0),
  _commonData(commonData)
{
  _updateFromTechnique();
}

void TechniquePropsWidget::_updateFromTechnique()
{
  //  Обновляем все старые виджеты
  for(Subwidgets::iterator iWidget = _subwidgets.begin();
      iWidget != _subwidgets.end();
      iWidget++)
  {
    iWidget->second->updateFromTechnique();
  }

  //  Добавляем виджеты для вновь появившихся свойств
  const mt::TechniqueConfiguration* configuration = _technique.configuration();
  if(configuration != nullptr)
  {
    // Виджеты для юниформов
    for(const mt::TechniqueConfiguration::UniformBuffer& buffer :
                                                  configuration->uniformBuffers)
    {
      for(const mt::TechniqueConfiguration::UniformVariable& variable :
                                                              buffer.variables)
      {
        _addSubwidget(variable.fullName, variable.shortName);
      }
    }

    // Виджеты для ресурсов
    for(const mt::TechniqueConfiguration::Resource& resource :
                                                      configuration->resources)
    {
      if(resource.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) continue;
      _addSubwidget(resource.name, resource.name);
    }

    _lastConfigurationRevision = configuration->revision;
  }
}

void TechniquePropsWidget::_addSubwidget( const std::string& fullName,
                                          const std::string& shortName)
{
  Subwidgets::const_iterator iWidget = _subwidgets.find(fullName);
  if(iWidget != _subwidgets.end()) return;

  std::unique_ptr<TechniquePropertyWidget> newWidget(
                                      new TechniquePropertyWidget(_technique,
                                                                  fullName,
                                                                  shortName,
                                                                  _commonData));
  _subwidgets[fullName] = std::move(newWidget);
}

void TechniquePropsWidget::makeGUI()
{
  const mt::TechniqueConfiguration* configuration = _technique.configuration();
  if( configuration == nullptr ||
      _lastConfigurationRevision != configuration->revision)
  {
    _updateFromTechnique();
  }

  mt::ImGuiPropertyGrid techniquePropsGrid("##techniqueProps");

  for(Subwidgets::iterator iWidget = _subwidgets.begin();
      iWidget != _subwidgets.end();
      iWidget++)
  {
    TechniquePropertyWidget* widget = iWidget->second.get();
    if(!widget->active()) continue;

    techniquePropsGrid.addRow(widget->shortName().c_str(),
                              widget->fullName().c_str());
    widget->makeGUI();
  }
}

void TechniquePropsWidget::save(YAML::Emitter& target) const
{
  target << YAML::BeginSeq;

  for ( Subwidgets::const_iterator iWidget = _subwidgets.begin();
        iWidget != _subwidgets.end();
        iWidget++)
  {
    const TechniquePropertyWidget* widget = iWidget->second.get();
    if(widget->active()) widget->save(target);
  }

  target << YAML::EndSeq;
}

void TechniquePropsWidget::load(const YAML::Node& source)
{
  if(!source.IsDefined() || !source.IsSequence()) return;

  //  Грузим настройки техники
  for(YAML::Node propertyNode : source)
  {
    //  Нам нужно полное и короткое имя, прежде чем мы сможем найти или создать
    //  настройку
    std::string fullPropertyName =
                            TechniquePropertyWidget::readFullName(propertyNode);
    if(fullPropertyName.empty()) continue;

    std::string shortPropertyName =
                          TechniquePropertyWidget::readShortName(propertyNode);
    if (shortPropertyName.empty()) shortPropertyName = fullPropertyName;

    //  Если настройки ещё нет, то добавим её
    _addSubwidget(fullPropertyName, shortPropertyName);

    //  Найдем и загрузим настройку
    Subwidgets::iterator iProperty = _subwidgets.find(fullPropertyName);
    MT_ASSERT(iProperty != _subwidgets.end());
    iProperty->second->load(propertyNode);
  }
}
