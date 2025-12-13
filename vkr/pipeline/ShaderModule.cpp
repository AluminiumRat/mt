#include <stdexcept>

#include <util/Abort.h>
#include <util/ContentLoader.h>
#include <util/Log.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/Device.h>

using namespace mt;

ShaderModule::ShaderModule( Device& device,
                            std::span<const char> spvData,
                            const char* debugName):
  _device(device),
  _handle(VK_NULL_HANDLE),
  _debugName(debugName)
{
  try
  {
    _createHandle(spvData);
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

ShaderModule::ShaderModule(Device& device, const char* spvFilename) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _debugName(spvFilename)
{
  try
  {
    std::vector<char> spvData =
                        ContentLoader::getContentLoader().loadData(spvFilename);
    _createHandle(spvData);
  }
  catch (...)
  {
    _cleanup();
    throw;
  }
}

void ShaderModule::_createHandle(std::span<const char> spvData)
{
  if(spvData.empty()) throw std::runtime_error(_debugName + ": SPIR-V data is empty");
  if(spvData.size() % 4 != 0) throw std::runtime_error(_debugName + ": unaligned SPIR-V data");

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = spvData.size();
  createInfo.pCode = (uint32_t*)spvData.data();
  if (vkCreateShaderModule( _device.handle(),
                            &createInfo,
                            nullptr,
                            &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error(std::string("ShaderModule: Failed to create shader module: ") + _debugName);
  }
}

ShaderModule::~ShaderModule() noexcept
{
  _cleanup();
}

void ShaderModule::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
