#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  //  Реестр для описаний COMMON дескриптер-сетов для техник.
  //  Нужен для того чтобы отделить загрузку и компилирование техники от
  //    конкретной реализации рендер системы
  //  Общий принцип действия: при старте приложения ренде-система регистрирует
  //    в этом реестре описания всех COMMON сетов, которые могут быть
  //    использованы (для разных типов кадров могут быть использованы разные
  //    коммон сеты). При сборке техники TechniqueConfigurator берет из этого 
  //    реестра описание COMMON сета по имени и строит из него лэйаут сета.
  //  ВНИМВНИЕ! Класс - синглтон. Регистрация не потокобезопасная. Регистрируйте
  //    все описания сетов на старте, до того как начнется сборка техник.
  class CommonSetRegistry
  {
  private:
    CommonSetRegistry() = default;
  public:
    CommonSetRegistry(const CommonSetRegistry&) = delete;
    CommonSetRegistry& operator = (const CommonSetRegistry&) = delete;
    ~CommonSetRegistry() noexcept = default;

    static void registerSet(
                        const char* setName,
                        std::span<const VkDescriptorSetLayoutBinding> bindings);

    static const std::vector<VkDescriptorSetLayoutBinding>&
                                          getSet(const char* setName) noexcept;
  private:
    static CommonSetRegistry& _instance();

  private:
    using Bindings = std::vector<VkDescriptorSetLayoutBinding>;
    using Sets = std::map<std::string, Bindings>;
    Sets _sets;
  };
}