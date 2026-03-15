#pragma once

#include <span>

#include "vku/scoped/device_waiter.hpp"

namespace vkit {
struct ComputeShaderProgramCreateInfo {
  vk::Device device;
  std::span<const std::uint32_t> computeSpirv;
  std::span<const vk::DescriptorSetLayout> setLayouts;
  std::span<const vk::PushConstantRange> pushConstantRanges;
};

class ComputeShaderProgram {
 public:
  using CreateInfo = ComputeShaderProgramCreateInfo;

  explicit ComputeShaderProgram(const CreateInfo& createInfo);

  void bind(vk::CommandBuffer cb) const;

  void dispatch(vk::CommandBuffer cb, std::uint32_t groupCountX,
                std::uint32_t groupCountY, std::uint32_t groupCountZ = 1) const;

 private:
  vk::UniqueShaderEXT shader_;
  vku::DeviceWaiter waiter_;
};
};  // namespace vkit
