#pragma once

#include "vku/scoped/device_waiter.hpp"

namespace vkit {
static constexpr auto kColorBlendEquationV = [] {
  auto ret = vk::ColorBlendEquationEXT{};
  ret.setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
  return ret;
}();

enum : std::uint8_t {
  kNone = 0,
  kAlphaBlend = 1 << 0,
  kDepthTest = 1 << 1,
};

static constexpr auto kFlagsV = kAlphaBlend | kDepthTest;

struct ShaderProgramCreateInfo {
  vk::Device device;
  std::span<const std::uint32_t> vertexSpirv;
  std::span<const std::uint32_t> fragmentSpirv;
  std::span<const vk::DescriptorSetLayout> setLayouts;
  std::span<const vk::PushConstantRange> pushConstantRanges;
};

class ShaderProgram {
 public:
  using CreateInfo = ShaderProgramCreateInfo;

  explicit ShaderProgram(const CreateInfo& createInfo);

  void bind(vk::CommandBuffer cb, glm::ivec2 framebufferSize,
            glm::ivec2 offset = {0, 0}) const;

  vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
  vk::PolygonMode polygonMode{vk::PolygonMode::eFill};
  float lineWidth{1.0F};

  vk::ColorBlendEquationEXT colorBlendEquation{kColorBlendEquationV};
  vk::CompareOp depthCompareOp{vk::CompareOp::eLessOrEqual};

  std::uint8_t flags{kFlagsV};

 private:
  static void setViewportScissor(vk::CommandBuffer cb, glm::ivec2 size,
                                 glm::ivec2 offset);
  static void setStaticStates(vk::CommandBuffer cb);
  void setCommonStates(vk::CommandBuffer cb) const;
  void setVertexStates(vk::CommandBuffer cb) const;
  void setFragmentStates(vk::CommandBuffer cb) const;
  void bindShaders(vk::CommandBuffer cb) const;

  std::vector<vk::UniqueShaderEXT> shaders_;

  vku::DeviceWaiter waiter_;
};

};  // namespace vkit
