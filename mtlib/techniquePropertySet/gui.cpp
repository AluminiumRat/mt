#include <imgui.h>

#include <gui/GUIWindow.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/modalDialogs.h>
#include <techniquePropertySet/gui.h>
#include <techniquePropertySet/techniqueProperty.h>
#include <techniquePropertySet/techniquePropertySet.h>
#include <util/vkMeta.h>

namespace fs = std::filesystem;

using namespace mt;

void propertyGui(TechniqueProperty& property);
void makeUniformGUI(TechniqueProperty& property);
void makeResourceGUI(TechniqueProperty& property);
void makeTextureGUI(TechniqueProperty& property);
void makeBufferGUI(TechniqueProperty& property);
void makeSamplerGUI(TechniqueProperty& property);
void samplerModeGUI(TechniqueProperty& property);
void customSamplerGUI(TechniqueProperty& property);

void mt::techniquePropertyGrid( const char* id,
                                TechniquePropertySet& propertySet)
{
  propertySet.updateFromTechnique();

  mt::ImGuiPropertyGrid techniquePropsGrid(id);

  propertySet.enumerateProperties(
    [&](TechniqueProperty& property)
    {
      if( !property.isActive() ||
          property.guiHints() & TechniqueConfiguration::GUI_HINT_HIDDEN)
      {
        return;
      }

      techniquePropsGrid.addRow(property.shortName().c_str(),
                                property.name().c_str());
      propertyGui(property);
    });
}

static void propertyGui(TechniqueProperty& property)
{
  if(property.unsupportedType())
  {
    ImGui::Text("Unsupported type");
    return;
  }

  if(property.isUniform())
  {
    makeUniformGUI(property);
    return;
  }

  if(property.isResource())
  {
    makeResourceGUI(property);
    return;
  }

  ImGui::Text("Unsupported type");
}

static void makeUniformGUI(TechniqueProperty& property)
{
  if(property.scalarType() == TechniqueConfiguration::INT_TYPE)
  {
    int newValue[] = { property.intValue()[0],
                       property.intValue()[1],
                       property.intValue()[2],
                       property.intValue()[3]};
    switch(property.vectorSize())
    {
      case 1:
        ImGui::InputInt(property.name().c_str(), newValue, 0);
        break;
      case 2:
        ImGui::InputInt2(property.name().c_str(), newValue, 0);
        break;
      case 3:
        ImGui::InputInt3(property.name().c_str(), newValue, 0);
        break;
      case 4:
        ImGui::InputInt4(property.name().c_str(), newValue, 0);
        break;
    }
    property.setUniformValue(glm::ivec4(newValue[0],
                                        newValue[1],
                                        newValue[2],
                                        newValue[3]));
  }
  else if(property.scalarType() == TechniqueConfiguration::FLOAT_TYPE)
  {
    glm::vec4 newValue = property.floatValue();
    switch(property.vectorSize())
    {
      case 1:
        ImGui::InputFloat(property.name().c_str(), (float*)&newValue, 0);
        break;
      case 2:
        ImGui::InputFloat2(property.name().c_str(), (float*)&newValue, 0);
        break;
      case 3:
        ImGui::InputFloat3(property.name().c_str(), (float*)&newValue, 0);
        break;
      case 4:
        ImGui::InputFloat4(property.name().c_str(), (float*)&newValue, 0);
        break;
    }
    property.setUniformValue(newValue);
  }
  else ImGui::Text("Unsupported type");
}

static void makeResourceGUI(TechniqueProperty& property)
{
  switch(property.resourceType())
  {
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    makeTextureGUI(property);
    break;
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    makeBufferGUI(property);
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
    makeSamplerGUI(property);
    break;
  default:
    ImGui::Text("Unsupported type");
  }
}

static void makeTextureGUI(TechniqueProperty& property)
{
  if(fileSelectionLine(property.name().c_str(), property.resourcePath()))
  {
    try
    {
      fs::path file =
              openFileDialog( GUIWindow::currentWindow(),
                              FileFilters{{ .expression = "*.dds",
                                            .description = "DDS image(*.dds)"},
                                          { .expression = "*.hdr",
                                            .description = "HDR image(*.hdr)"},
                                          { .expression = "*.png",
                                            .description = "PNG image(*.png)"},
                                          { .expression = "*.*",
                                            .description = "All files(*.*)"}},
                              "");
      if(!file.empty()) property.setResourcePath(file);
    }
    catch (std::exception& error)
    {
      Log::error() << error.what();
      errorDialog(GUIWindow::currentWindow(), "Error", "Unable to open texture file");
    }
  }
}

static void makeBufferGUI(TechniqueProperty& property)
{
  if(fileSelectionLine(property.name().c_str(), property.resourcePath()))
  {
    try
    {
      fs::path file =
              openFileDialog(
                          GUIWindow::currentWindow(),
                          FileFilters{{ .expression = "*.bin",
                                        .description = "Binary files(*.bin)"}},
                          "");
      if(!file.empty()) property.setResourcePath(file);
    }
    catch (std::exception& error)
    {
      Log::error() << error.what();
      errorDialog(GUIWindow::currentWindow(), "Error", "Unable to open file");
    }
  }
}

static void makeSamplerGUI(TechniqueProperty& property)
{
  samplerModeGUI(property);
  const TechniqueProperty::SamplerValue* sampler = property.samplerValue();
  MT_ASSERT(sampler != nullptr);
  if(sampler->mode == TechniqueProperty::CUSTOM_SAMPLER_MODE)
  {
    customSamplerGUI(property);
  }
}

static void samplerModeGUI(TechniqueProperty& property)
{
  const TechniqueProperty::SamplerValue* sampler = property.samplerValue();
  MT_ASSERT(sampler != nullptr);

  TechniqueProperty::SamplerMode samplerMode = sampler->mode;

  static const Bimap<TechniqueProperty::SamplerMode> modeMap{
    "Sampler mode",
    {
      {TechniqueProperty::DEFAULT_SAMPLER_MODE, "Default"},
      {TechniqueProperty::CUSTOM_SAMPLER_MODE, "Custom"}
    }};
  if(enumSelectionCombo("##samplerMode", samplerMode, modeMap))
  {
    TechniqueProperty::SamplerValue newValue = *sampler;
    newValue.mode = samplerMode;
    property.setSamplerValue(newValue);
  }
}

static void customSamplerGUI(TechniqueProperty& property)
{
  MT_ASSERT(property.samplerValue() != nullptr);

  TechniqueProperty::SamplerValue sampler = *property.samplerValue();
  SamplerDescription& description = sampler.description;

  bool update = false;

  ImGuiPropertyGrid samplerGrid("##samplerProps");
  samplerGrid.addRow("Min filter:");
  update |= enumSelectionCombo( "##Minfilter",
                                description.minFilter,
                                filterMap);

  samplerGrid.addRow("Mag filter:");
  update |= enumSelectionCombo( "##Magfilter",
                                description.magFilter,
                                filterMap);

  samplerGrid.addRow("Mipmap mode:");
  update |= enumSelectionCombo( "##MipmapMode",
                                description.mipmapMode,
                                mipmapModeMap);

  samplerGrid.addRow("Min LOD:");
  update |= ImGui::InputFloat("##MinLod", &description.minLod);

  samplerGrid.addRow("Max LOD:");
  update |= ImGui::InputFloat("##MaxLod", &description.maxLod);

  samplerGrid.addRow("LOD bias:");
  update |= ImGui::InputFloat("##LODBias", &description.mipLodBias);

  samplerGrid.addRow("Anisotropy:");
  update |= ImGui::Checkbox("##AnisotropyEnable",
    &description.anisotropyEnable);

  samplerGrid.addRow("Max anisotropy:");
  update |= ImGui::InputFloat("##MaxAnisotropy", &description.maxAnisotropy);

  samplerGrid.addRow("Address U:");
  update |= enumSelectionCombo( "##AddressModeU",
                                description.addressModeU,
                                addressModeMap);

  samplerGrid.addRow("Address V:");
  update |= enumSelectionCombo( "##AddressModeV",
                                description.addressModeV,
                                addressModeMap);

  samplerGrid.addRow("Address W:");
  update |= enumSelectionCombo( "##AddressModeW",
                                description.addressModeW,
                                addressModeMap);

  samplerGrid.addRow("Compare enable:");
  update |= ImGui::Checkbox("##CompareEnable", &description.compareEnable);

  samplerGrid.addRow("Compare op:");
  update |= enumSelectionCombo( "##CompareOp",
                                description.compareOp,
                                compareOpMap);

  samplerGrid.addRow("Border:");
  update |= enumSelectionCombo( "##Border",
                                description.borderColor,
                                borderColorMap);

  samplerGrid.addRow("UCoord:", "Unnormalized coordinates");
  update |= ImGui::Checkbox("##UCoord", &description.unnormalizedCoordinates);

  if(update) property.setSamplerValue(sampler);
}