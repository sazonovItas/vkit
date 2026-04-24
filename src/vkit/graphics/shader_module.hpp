#pragma once

#include <filesystem>

namespace vkit::graphics {

class ShaderModule : public vk::UniqueShaderModule {
 public:
  explicit ShaderModule(vk::Device device, std::span<const std::uint32_t> code)
      : vk::UniqueShaderModule{device.createShaderModuleUnique(
            createShaderModuleCreateInfo(code))} {}

  auto stageCreateInfo(vk::ShaderStageFlagBits stage)
      -> vk::PipelineShaderStageCreateInfo {
    return {{}, stage, this->get(), "main", nullptr, nullptr};
  }

 private:
  static auto createShaderModuleCreateInfo(std::span<const std::uint32_t> code)
      -> vk::ShaderModuleCreateInfo {
    auto ci = vk::ShaderModuleCreateInfo{};
    ci.setPCode(code.data()).setCodeSize(code.size_bytes());
    return ci;
  };
};

class SpirVShaderModule : public ShaderModule {
 public:
  explicit SpirVShaderModule(vk::Device device,
                             std::span<const std::uint32_t> code)
      : ShaderModule{device, code} {};

  explicit SpirVShaderModule(vk::Device device,
                             const std::filesystem::path& path);
};

};  // namespace vkit::graphics
