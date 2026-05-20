#include "vkit/graphics/instance.hpp"

#include <print>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace vkit::graphics {

namespace {

constexpr auto kVkMajor = 1;
constexpr auto kVkMinor = 2;
constexpr auto kVkPatch = 0;

constexpr auto kVkVersionV = VK_MAKE_VERSION(kVkMajor, kVkMinor, kVkPatch);

[[nodiscard]] auto getLayers(std::span<char const* const> desired)
    -> std::vector<char const*> {
  auto ret = std::vector<char const*>{};
  ret.reserve(desired.size());

  auto const available = vk::enumerateInstanceLayerProperties();
  for (char const* layer : desired) {
    auto const pred = [layer = std::string_view{layer}](
                          vk::LayerProperties const& properties) {
      return properties.layerName == layer;
    };

    if (std::ranges::find_if(available, pred) == available.end()) {
      std::println("[VULKAN] [WARNING] Vulkan layer '{}' not found", layer);
      continue;
    }

    ret.push_back(layer);
  }

  return ret;
}

vk::Bool32 VKAPI_CALL
debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
              vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
              const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* /*pUserData*/) {
  if (pCallbackData && pCallbackData->pMessage) {
    std::println("[VULKAN] [VALIDATION]: {}", pCallbackData->pMessage);
  }

  return vk::False;
}

};  // namespace

Instance::Instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  auto const loader_version = vk::enumerateInstanceVersion();
  if (loader_version < kVkVersionV) {
    throw std::runtime_error{
        std::format("[VULKAN] Loader doesn't support vulkan {}.{}.{}", kVkMajor,
                    kVkMinor, kVkPatch),
    };
  }

  auto app_info = vk::ApplicationInfo{};
  app_info.setPApplicationName("vkit").setApiVersion(kVkVersionV);

  auto extensions = Window::getVulkanExtensions();

#ifndef NDEBUG
  extensions.push_back(vk::EXTDebugUtilsExtensionName);
  static constexpr auto kLayersV = std::array{"VK_LAYER_KHRONOS_validation"};
  const auto layers = getLayers(kLayersV);
#else
  const auto layers = std::vector<const char*>{};
#endif

  auto instance_ci = vk::InstanceCreateInfo{};
  instance_ci.setPApplicationInfo(&app_info)
      .setPEnabledLayerNames(layers)
      .setPEnabledExtensionNames(extensions);

#ifndef NDEBUG
  auto debug_ci = vk::DebugUtilsMessengerCreateInfoEXT{};
  debug_ci
      .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
      .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
      .setPfnUserCallback(&debugCallback);

  auto ci = vk::StructureChain{instance_ci, debug_ci};
  instance_ = vk::createInstanceUnique(ci.get());

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

  debugMessenger_ = instance_->createDebugUtilsMessengerEXTUnique(
      debug_ci, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
#else
  instance_ = vk::createInstanceUnique(instance_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
#endif
}

};  // namespace vkit::graphics
