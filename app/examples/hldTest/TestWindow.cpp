#include <ddsSupport/ddsSupport.h>
#include <hld/FrameContext.h>
#include <hld/HLDLib.h>
#include <technique/TechniqueLoader.h>
#include <vkr/Device.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Test window"),
  _frameTypeIndex(HLDLib::instance().getFrameTypeIndex(colorFrameType)),
  _meshAsset(new MeshAsset("Test mesh")),
  _drawable1(*_meshAsset),
  _drawable2(*_meshAsset),
  _drawable3(*_meshAsset),
  _commandMemoryPool(4 * 1024)
{
  _camera.setPerspectiveProjection(1, 1, 0.1f, 100);

  _setupMeshAsset();

  _scene.addDrawable(_drawable1);
  _scene.addDrawable(_drawable2);
  _scene.addDrawable(_drawable3);
}

void TestWindow::_setupMeshAsset()
{
  //  Техника отрисовки меша
  Ref configurator(new TechniqueConfigurator(device(), "Test technique"));
  loadConfigurator(*configurator, "examples/mesh/technique.tch");
  configurator->rebuildConfiguration();
  Ref technique(new Technique(*configurator));

  //  Создаем вершинный буфер
  glm::vec4 vertices[4] =  {{-0.5f, -0.5f, 0.0f, 1.0f},
                            {-0.5f,  0.5f, 0.0f, 1.0f},
                            { 0.5f, -0.5f, 0.0f, 1.0f},
                            { 0.5f,  0.5f, 0.0f, 1.0f}};
  Ref vertexBuffer(new DataBuffer(device(),
                                  sizeof(vertices),
                                  DataBuffer::STORAGE_BUFFER,
                                  "Vertex buffer"));
  device().graphicQueue()->uploadToBuffer(*vertexBuffer,
                                          0,
                                          sizeof(vertices),
                                          vertices);
  technique->getOrCreateResourceBinding("vertices").setBuffer(vertexBuffer);

  //  Создаем текстуру
  Ref<Image> image = loadDDS( "examples/image.dds",
                              device(),
                              nullptr,
                              true);
  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  technique->getOrCreateResourceBinding("colorTexture").setImage(imageView);

  //  Конфигурация ассета
  MeshAsset::Configuration meshConfig{};
  meshConfig.technique = technique;
  meshConfig.passes.push_back(MeshAsset::PassConfig{
                                        .frameTypeName = colorFrameType,
                                        .stageName = TestDrawStage::stageName,
                                        .layer = 0,
                                        .passName = "RenderPass"});
  meshConfig.vertexCount = sizeof(vertices) / sizeof(vertices[0]);
  meshConfig.maxInstancesCount = 2;

  _meshAsset->setConfiguration(meshConfig);
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  _drawPlan.clear();
  _commandMemoryPool.reset();

  _scene.fillDrawPlan(_drawPlan, _camera, _frameTypeIndex);

  FrameContext frameContext{};
  frameContext.frameTypeIndex = _frameTypeIndex;
  frameContext.drawPlan = &_drawPlan;
  frameContext.commandMemoryPool = &_commandMemoryPool;
  frameContext.frameBuffer = &frameBuffer;
  frameContext.commandProducer = &commandProducer;

  _drawStage.draw(frameContext);
}
