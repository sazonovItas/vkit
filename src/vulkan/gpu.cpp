#include "gpu.hpp"

#include <algorithm>
#include <print>
#include <ranges>
#include <unordered_set>

#include "util.hpp"
#include "vk_mem_alloc.hpp"

namespace {
constexpr std::array kRequiredExtensions{
    vk::KHRSwapchainExtensionName,
    vk::EXTShaderObjectExtensionName,
};

constexpr auto kRequiredFeatures = vk::PhysicalDeviceFeatures{}
                                       .setFillModeNonSolid(vk::True)
                                       .setWideLines(vk::True)
                                       .setSamplerAnisotropy(vk::True)
                                       .setSampleRateShading(vk::True);
};  // namespace

namespace vkit::vulkan {
QueueFamilies::QueueFamilies(vk::PhysicalDevice physical_device,
                             vk::SurfaceKHR surface) {
  const auto queue_family_properties =
      physical_device.getQueueFamilyProperties();

  compute = [&] -> std::uint32_t {
    for (const auto &[idx, props] :
         queue_family_properties | std::ranges::views::enumerate) {
      if (util::contains(props.queueFlags, vk::QueueFlagBits::eCompute) &&
          !util::contains(props.queueFlags, vk::QueueFlagBits::eGraphics)) {
        return idx;
      }
    }

    for (const auto &[idx, props] :
         queue_family_properties | std::ranges::views::enumerate) {
      if (util::contains(props.queueFlags, vk::QueueFlagBits::eCompute)) {
        return idx;
      }
    }

    std::unreachable();
  }();

  graphicsPresent = [&] -> std::uint32_t {
    for (const auto &[idx, props] :
         queue_family_properties | std::ranges::views::enumerate) {
      if (util::contains(props.queueFlags, vk::QueueFlagBits::eGraphics) &&
          physical_device.getSurfaceSupportKHR(idx, surface) == vk::True) {
        return idx;
      }
    }

    throw std::runtime_error{"the GPU does not have graphics capability"};
  }();

  transfer = [&] -> std::uint32_t {
    for (const auto &[idx, props] :
         queue_family_properties | std::ranges::views::enumerate) {
      if (util::contains(props.queueFlags, vk::QueueFlagBits::eTransfer) &&
          !util::contains(props.queueFlags, vk::QueueFlagBits::eGraphics) &&
          !util::contains(props.queueFlags, vk::QueueFlagBits::eCompute)) {
        return idx;
      }
    }

    return compute;
  }();

  uniqueIndices = {compute, graphicsPresent, transfer};
  std::ranges::sort(uniqueIndices);
  const auto ret = std::ranges::unique(uniqueIndices);
  uniqueIndices.erase(ret.begin(), ret.end());
}

Queues::Queues(vk::Device device, const QueueFamilies &queue_families) noexcept
    : compute{device.getQueue(queue_families.compute, 0)},
      graphicsPresent{device.getQueue(queue_families.graphicsPresent, 0)},
      transfer{device.getQueue(queue_families.transfer, 0)} {}

Gpu::Gpu(const vk::Instance &instance, vk::SurfaceKHR surface)
    : physicalDevice{selectGpu(instance, surface)},
      properties{physicalDevice.getProperties()},
      queueFamilies(physicalDevice, surface),
      device{createDevice()},
      queues{*device, queueFamilies},
      allocator{createAllocator(instance)} {
  // TODO(itas): move this log somewhere else
  // TODO(itas): add options for eVirtual, eCpu and eOther
  std::println("Selected GPU: {}, type: {}",
               std::string_view{properties.deviceName},
               properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
                   ? "DiscreteGpu"
                   : "IntegratedGpu");
}

auto Gpu::createCommandPool(const std::uint32_t queueFamilyIndex,
                            vk::CommandPoolCreateFlagBits flags)
    -> vk::UniqueCommandPool {
  auto it = std::ranges::find(queueFamilies.uniqueIndices, queueFamilyIndex);
  if (it == queueFamilies.uniqueIndices.end()) {
    throw std::runtime_error{std::format(
        "failed to create command pool: unknown queue family index `{}`",
        queueFamilyIndex)};
  }

  auto ci = vk::CommandPoolCreateInfo{};
  ci.setQueueFamilyIndex(queueFamilyIndex).setFlags(flags);

  return device->createCommandPoolUnique(ci);
}

auto Gpu::selectGpu(const vk::Instance &instance, vk::SurfaceKHR surface) const
    -> vk::PhysicalDevice {
  const auto gpu_rater = [&](vk::PhysicalDevice gpu) -> std::uint32_t {
    try {
      std::ignore = QueueFamilies{gpu, surface};
    } catch (const std::runtime_error &) {
      return 0U;
    }

    const auto available_extensions = gpu.enumerateDeviceExtensionProperties();

    auto available_extensions_names =
        available_extensions |
        std::views::transform([](const vk::ExtensionProperties &properties) {
          return static_cast<std::string_view>(properties.extensionName);
        }) |
        std::ranges::to<std::vector>();
    std::ranges::sort(available_extensions_names);

    auto required_extensions_names = kRequiredExtensions |
                                     std::views::transform([](const char *str) {
                                       return std::string_view{str};
                                     }) |
                                     std::ranges::to<std::vector>();
    std::ranges::sort(required_extensions_names);

    if (!std::ranges::includes(available_extensions_names,
                               required_extensions_names)) {
      return 0U;
    }

    const auto [features2, vulkan11_features, vulkan12_features] =
        gpu.getFeatures2<vk::PhysicalDeviceFeatures2,
                         vk::PhysicalDeviceVulkan11Features,
                         vk::PhysicalDeviceVulkan12Features>();

    const auto gpu_properties =
        gpu.getProperties2<vk::PhysicalDeviceProperties2,
                           vk::PhysicalDeviceSubgroupProperties>();

    const auto &properties =
        gpu_properties.get<vk::PhysicalDeviceProperties2>().properties;

    auto score = std::uint32_t{0};
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      score += 1000;
    }

    score += properties.limits.maxImageDimension2D;

    return score;
  };

  auto gpus = instance.enumeratePhysicalDevices();

  auto best_gpu = *std::ranges::max_element(gpus, {}, gpu_rater);
  if (gpu_rater(best_gpu) == 0U) {
    throw std::runtime_error{"no suitable GPU for the application"};
  }

  auto properties = best_gpu.getProperties();

  return best_gpu;
}

auto Gpu::createDevice() -> vk::UniqueDevice {
  const std::vector available_extensions =
      physicalDevice.enumerateDeviceExtensionProperties();
  const std::unordered_set available_extensions_names =
      available_extensions |
      std::views::transform([](const vk::ExtensionProperties &properties) {
        return static_cast<std::string_view>(properties.extensionName);
      }) |
      std::ranges::to<std::unordered_set>();

  auto extensions = std::vector{std::from_range, kRequiredExtensions};

  auto queue_cis = std::vector<vk::DeviceQueueCreateInfo>();
  queue_cis.reserve(queueFamilies.uniqueIndices.size());

  constexpr static auto kPriority = 1.F;
  for (std::uint32_t idx : queueFamilies.uniqueIndices) {
    auto ci =
        vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(idx).setQueuePriorities(
            vk::ArrayProxyNoTemporaries<const float>{kPriority});
    queue_cis.push_back(ci);
  }

  auto ci = vk::StructureChain{
      vk::DeviceCreateInfo{}
          .setQueueCreateInfos(queue_cis)
          .setPEnabledExtensionNames(extensions),
      vk::PhysicalDeviceFeatures2{kRequiredFeatures},
      vk::PhysicalDeviceSynchronization2Features{vk::True},
      vk::PhysicalDeviceDynamicRenderingFeatures{vk::True},
      vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True},
      vk::PhysicalDeviceVulkan12Features{}
          .setScalarBlockLayout(vk::True)
          .setBufferDeviceAddress(vk::True),
      vk::PhysicalDeviceDescriptorIndexingFeatures{}
          .setRuntimeDescriptorArray(vk::True)
          .setDescriptorBindingPartiallyBound(vk::True)
          .setShaderStorageBufferArrayNonUniformIndexing(vk::True)
          .setShaderSampledImageArrayNonUniformIndexing(vk::True)
          .setShaderStorageImageArrayNonUniformIndexing(vk::True)
          .setDescriptorBindingStorageBufferUpdateAfterBind(vk::True)
          .setDescriptorBindingSampledImageUpdateAfterBind(vk::True)
          .setDescriptorBindingStorageImageUpdateAfterBind(vk::True),
  };

  auto device = physicalDevice.createDeviceUnique(ci.get());
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
}

auto Gpu::createAllocator(const vk::Instance &instance) const
    -> vma::Allocator {
  auto const &dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;

  auto vma_vk_funcs =
      vma::VulkanFunctions{}
          .setVkGetInstanceProcAddr(dispatcher.vkGetInstanceProcAddr)
          .setVkGetDeviceProcAddr(dispatcher.vkGetDeviceProcAddr);

  auto ci = vma::AllocatorCreateInfo{}
                .setInstance(instance)
                .setPhysicalDevice(physicalDevice)
                .setDevice(*device)
                .setPVulkanFunctions(&vma_vk_funcs)
                .setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress |
                          vma::AllocatorCreateFlagBits::eKhrMaintenance5);

  return vma::createAllocator(ci);
}
};  // namespace vkit::vulkan
