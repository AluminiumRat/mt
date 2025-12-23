#include <Technique/Technique.h>

#include <imgui.h>

#include <TechniquePropsWidget.h>

TechniquePropsWidget::TechniquePropsWidget(mt::Technique& technique) :
  _technique(technique),
  _lastConfigurationRevision(0)
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
                  new TechniquePropertyWidget(_technique, fullName, shortName));
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

  if(!ImGui::BeginTable("##techniqueProps", 2, 0)) return;
  ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(1);
  ImGui::PushItemWidth(-FLT_MIN);

  for(Subwidgets::iterator iWidget = _subwidgets.begin();
      iWidget != _subwidgets.end();
      iWidget++)
  {
    TechniquePropertyWidget* widget = iWidget->second.get();
    if(!widget->active()) continue;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(widget->shortName().c_str());
    ImGui::SetItemTooltip(widget->fullName().c_str());

    ImGui::TableSetColumnIndex(1);
    widget->makeGUI();
  }

  ImGui::EndTable();
}
