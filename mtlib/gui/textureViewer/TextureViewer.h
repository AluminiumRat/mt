#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <gui/cameraManipulator/FlatCameraManipulator.h>
#include <gui/cameraManipulator/OrbitalCameraManipulator.h>
#include <technique/Technique.h>
#include <util/Camera.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/FrameBuffer.h>
#include <vkr/Sampler.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class TechniqueManager;

  //  Класс, который реализует конкретный вьювер-виджет
  //  Можно создать и использовать этот класс непосредственно, но также можно
  //    воспользоваться статическим методом "makeGUIIm" для вставки
  //    вьювера в immediately mode стиле
  class TextureViewer
  {
  public:
    //  Если techniqueManager не nullptr, то она будет использована для
    //    загрузки техники отрисовки виджета(асинхронно). В противном случае
    //    техника будет загружена в конструкторе синхронно.
    TextureViewer(Device& device, TechniqueManager* techniqueManager);
    TextureViewer(const TextureViewer&) = delete;
    TextureViewer& operator = (const TextureViewer&) = delete;
    ~TextureViewer() noexcept;

    //  Добавить вьювер в ImGui контекст. Нестатический метод, пользователь
    //    сам управляет объектом класса TextureViewer
    //  image должна либо иметь включенный автоконтроль лэйаута, либо находиться
    //    в лэйауте VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void makeGUI( const char* id,
                  const Image& image,
                  ImVec2 size = ImVec2(-FLT_MIN, -FLT_MIN));

    //  Добавить вьювер в ImGui контекст. Cтатический метод в стиле ImGui,
    //    не требует явного создания и управления объектом TextureViewer
    //  image должна либо иметь включенный автоконтроль лэйаута, либо находиться
    //    в лэйауте VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    //  ВНИМАНИЕ! Метод обращается к TextureViewerManager. Убедитесь, что
    //    TextureViewerManager создан и для него вызывается flush
    static void makeGUIIm(const char* id,
                          const Image& image,
                          ImVec2 size = ImVec2(-FLT_MIN, -FLT_MIN));

  private:
    enum ViewType
    {
      FLAT_VIEW,
      CUBEMAP_VIEW,
      INV_CUBEMAP_VIEW
    };

    enum SamplerType
    {
      NEAREST_SAMPLER,
      LINEAR_SAMPLER
    };

  private:
    void _loadViewTechnique(TechniqueManager* techniqueManager);
    //  Виджеты управления сверху от основного окна просмотра
    void _makeControlWidgets();
    //  Комбо бокс для выбора типа отрисовки
    void _viewTypeCombo();
    //  Контрол для выбора слоя текстурного массива
    void _layerInput();

    //  Создет область, над которой бедет работать манипулятор камеры
    void _makeUndraggedArea(glm::uvec2 widgetSize);

    //  Определить реальный размер виджета по размеру, переданному пользователем
    glm::uvec2 _getWidgetSize(const ImVec2& userSize) const;

    //  Настроить flat view так, чтобы вся текстура влезала во вьювер
    void _adjustFlatView(glm::uvec2 widgetSize);

    void _rebuildRenderTarget(glm::uvec2 widgetSize);
    void _setNewRenderedImage(const Image& image);
    void _clearRenderTarget() noexcept;

    void _updateRenderParams();
    void _renderScene();
    void _drawGeometry(CommandProducerGraphic& producer);

  private:
    Device& _device;

    std::unique_ptr<Technique> _viewTechnique;
    TechniquePass* _flatPass;
    TechniquePass* _cubemapPass;
    TechniquePass* _invCubemapPass;
    ResourceBinding* _flatTexture;
    ResourceBinding* _cubemapTexture;
    UniformVariable* _viewProjectionMatrix;
    UniformVariable* _modelMatrix;
    UniformVariable* _brightnessUniform;
    UniformVariable* _mipUniform;
    UniformVariable* _layerUniform;
    Selection* _samplerSelection;

    ConstRef<Image> _renderedImage;
    bool _cubemapAllowed;

    Ref<Image> _renderTargetImage;
    Ref<FrameBuffer> _frameBuffer;

    Camera _viewCamera;
    FlatCameraManipulator _flatManipulator;
    OrbitalCameraManipulator _orbitalManipulator;

    ViewType _viewType;
    SamplerType _samplerType;
    float _brightness;
    int _mipIndex;
    int _layerIndex;

    //  Объекты, которые нужны для отрисовки _renderTargetImage в окно ImGui
    Ref<Sampler> _imGuiSampler;
    Ref<DescriptorSet> _imGuiDescriptorSet;
  };
}