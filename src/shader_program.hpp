#pragma once

#include "scoped_waiter.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {

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
  std::span<std::uint32_t const> vertex_spirv;
  std::span<std::uint32_t const> fragment_spirv;
  std::span<vk::DescriptorSetLayout const> set_layouts;
  std::span<vk::PushConstantRange const> push_constant_ranges;
};

struct PushConstants {
  vk::DeviceAddress vertex_buffer;
};

class ShaderProgram {
 public:
  using CreateInfo = ShaderProgramCreateInfo;

  explicit ShaderProgram(CreateInfo const& create_info);

  void bind(vk::CommandBuffer command_buffer, glm::ivec2 offset,
            glm::ivec2 framebuffer_size) const;

  vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
  vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
  float line_width{1.0F};

  vk::ColorBlendEquationEXT color_blend_equation{kColorBlendEquationV};
  vk::CompareOp depth_compare_op{vk::CompareOp::eLess};

  std::uint8_t flags{kFlagsV};

 private:
  static void set_viewport_scissor(vk::CommandBuffer command_buffer,
                                   glm::ivec2 offset,
                                   glm::ivec2 framebuffer_size);
  static void set_static_states(vk::CommandBuffer command_buffer);
  void set_common_states(vk::CommandBuffer command_buffer) const;
  void set_vertex_states(vk::CommandBuffer command_buffer) const;
  void set_fragment_states(vk::CommandBuffer command_buffer) const;
  void bind_shaders(vk::CommandBuffer command_buffer) const;

  std::vector<vk::UniqueShaderEXT> m_shaders_;

  ScopedWaiter m_waiter_;
};

};  // namespace lvk
