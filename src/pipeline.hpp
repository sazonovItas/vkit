#pragma once

#include <cstdint>

namespace vkit {
struct PipelineFlag {
  enum : std::uint8_t {
    kNone = 0,
    kAlphaBlend = 1 << 0,
    kDepthTest = 1 << 1,
  };
};

struct PipelineState {
  using Flag = PipelineFlag;

  [[nodiscard]] static constexpr auto defaultFlags() -> std::uint8_t {
    return Flag::kAlphaBlend | Flag::kDepthTest;
  }

  vk::ShaderModule vertexShader;
  vk::ShaderModule fragmentShader;

  vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
  vk::PolygonMode polygonMode{vk::PolygonMode::eFill};
  vk::CullModeFlags cullMode{vk::CullModeFlagBits::eNone};
  vk::CompareOp depthCompare{vk::CompareOp::eLess};
  std::uint8_t flags{defaultFlags()};
};

struct PipelineBuilderCreateInfo {
  vk::Device device;
  vk::SampleCountFlagBits samples;
  vk::Format colorFormat;
  vk::Format depthFormat;
};

class PipelineBuilder {
 public:
  using CreateInfo = PipelineBuilderCreateInfo;

  explicit PipelineBuilder(const CreateInfo& createInfo) : ci_{createInfo} {}

  [[nodiscard]] auto build(vk::PipelineLayout layout,
                           const PipelineState& state) const
      -> vk::UniquePipeline;

 private:
  CreateInfo ci_;
};

};  // namespace vkit
