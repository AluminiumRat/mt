#include <fstream>
#include <memory>

#include <vulkan/vulkan.h>

#include <ddsSupport/ddsSupport.h>
#include <technique/TechniqueLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Test window"),
  _configurator(new TechniqueConfigurator(device, "Test technique")),
  _technique(new Technique(*_configurator)),
  _asyncQueue(nullptr),
  _textureManager(_fileWatcher, _asyncQueue),
  _bufferManager(_fileWatcher, _asyncQueue),
  _pass(_technique->getOrCreatePass("RenderPass")),
  _vertexBuffer(_technique->getOrCreateResourceBinding("vertices")),
  _texture(_technique->getOrCreateResourceBinding("colorTexture"))
{
  _makeConfiguration();
  _createVertexBuffer();
  _createTexture();
}

void TestWindow::_makeConfiguration()
{
  loadConfigurator(*_configurator, "examples/dds_load/technique.tch");
  _configurator->rebuildConfiguration();
}

void TestWindow::_createVertexBuffer()
{
  // Можно закоментить, чтобы создать буфер вречную и сохранить его в файл
  #define LOAD_VERTEX_BUFFER_FROM_FILE

  #ifndef LOAD_VERTEX_BUFFER_FROM_FILE
    struct Vertex
    {
      alignas(16) glm::vec3 position;
    };
    Vertex vertices[4] =  { {.position = {-0.5f, -0.5f, 0.0f}},
                            {.position = {-0.5f, 0.5f, 0.0f}},
                            {.position = {0.5f, -0.5f, 0.0f}},
                            {.position = {0.5f, 0.5f, 0.0f}}};

    std::ofstream fileStraem("square2.bin", std::ios::binary);
    if (!fileStraem.is_open()) throw std::runtime_error("Unable to write to square.bin");
    fileStraem.write((const char*)vertices, sizeof(vertices));

    Ref<DataBuffer> buffer(new DataBuffer(device(),
                                          sizeof(vertices),
                                          DataBuffer::STORAGE_BUFFER,
                                          "Vertex buffer"));
    device().graphicQueue()->uploadToBuffer(*buffer,
                                            0,
                                            sizeof(vertices),
                                            vertices);
    _vertexBuffer.setBuffer(buffer);
  #else
    ConstRef<TechniqueResource> vertexBufferResource =
                                    _bufferManager.loadImmediately(
                                                    "examples/square.bin",
                                                    *device().graphicQueue());

    /*ConstRef<TechniqueResource> vertexBufferResource =
                                    _bufferManager.sheduleLoading(
                                                    "examples/square.bin",
                                                    *device().graphicQueue());*/

    _vertexBuffer.setResource(vertexBufferResource);
  #endif
}

void TestWindow::_createTexture()
{
  //  Здесь можно выбрать, как именно загружать текстуру

  /*ConstRef<TechniqueResource> texture = _textureManager.sheduleLoading(
                                                      "examples/image.dds",
                                                      *device().graphicQueue(),
                                                      true);*/

  ConstRef<TechniqueResource> texture = _textureManager.loadImmediately(
                                                      "examples/image.dds",
                                                      *device().graphicQueue(),
                                                      true);
  _texture.setResource(texture);
}

void TestWindow::update()
{
  _asyncQueue.update();
  _fileWatcher.propagateChanges();
  RenderWindow::update();
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  Technique::Bind bind(*_technique, _pass, commandProducer);
  if (bind.isValid())
  {
    commandProducer.draw(4);
    bind.release();
  }

  renderPass.endPass();
}
