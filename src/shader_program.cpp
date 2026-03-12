#include "shader_program.hpp"

#include <array>
#include <stdexcept>

#include "render_target.hpp"
#include "vku/utils/utils.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit {
ShaderProgram::ShaderProgram(const CreateInfo& createInfo) {
  auto const create_shader_ci =
      [&createInfo](std::span<std::uint32_t const> spirv) {
        auto ret = vk::ShaderCreateInfoEXT{};
        ret.setCodeSize(spirv.size_bytes())
            .setPCode(spirv.data())
            .setSetLayouts(createInfo.setLayouts)
            .setPushConstantRanges(createInfo.pushConstantRanges)
            .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
            .setPName("main");
        return ret;
      };

  auto shader_cis = std::array{
      create_shader_ci(createInfo.vertexSpirv),
      create_shader_ci(createInfo.fragmentSpirv),
  };

  shader_cis[0]
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setNextStage(vk::ShaderStageFlagBits::eFragment);
  shader_cis[1].setStage(vk::ShaderStageFlagBits::eFragment);

  auto result = createInfo.device.createShadersEXTUnique(shader_cis);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to create Shader Objects"};
  }

  shaders_ = std::move(result.value);
  waiter_ = createInfo.device;
}

void ShaderProgram::bind(vk::CommandBuffer cb, glm::ivec2 framebufferSize,
                         glm::ivec2 offset) const {
  setViewportScissor(cb, framebufferSize, offset);
  setStaticStates(cb);
  setCommonStates(cb);
  setVertexStates(cb);
  setFragmentStates(cb);
  bindShaders(cb);
}

void ShaderProgram::setViewportScissor(vk::CommandBuffer cb, glm::ivec2 size,
                                       glm::ivec2 offset) {
  const auto fsize = glm::vec2{size};
  auto viewport = vk::Viewport{};
  viewport.setX(offset.x)
      .setY(fsize.y + offset.y)
      .setWidth(fsize.x)
      .setHeight(-fsize.y)
      .setMinDepth(0.0F)
      .setMaxDepth(1.0F);
  cb.setViewportWithCount(viewport);

  const auto usize = glm::uvec2{size};
  const auto scissor =
      vk::Rect2D{vk::Offset2D{}, vk::Extent2D{usize.x, usize.y}};
  cb.setScissorWithCount(scissor);
}
void ShaderProgram::setStaticStates(vk::CommandBuffer cb) {
  cb.setRasterizerDiscardEnable(vk::False);
  cb.setRasterizationSamplesEXT(kSampleCount);
  cb.setSampleMaskEXT(kSampleCount, 0xff);
  cb.setAlphaToCoverageEnableEXT(vk::False);
  cb.setFrontFace(vk::FrontFace::eCounterClockwise);
  cb.setCullMode(vk::CullModeFlagBits::eBack);
  cb.setDepthBoundsTestEnable(vk::False);
  cb.setDepthBiasEnable(vk::False);
  cb.setDepthBounds(0.0F, 1.0F);
  cb.setStencilTestEnable(vk::False);
  cb.setPrimitiveRestartEnable(vk::False);
  cb.setColorWriteMaskEXT(0, ~vk::ColorComponentFlagBits{});
}

void ShaderProgram::setCommonStates(vk::CommandBuffer cb) const {
  const auto depth_test = vku::toVkbool((flags & kDepthTest) == kDepthTest);
  cb.setDepthWriteEnable(depth_test);
  cb.setDepthTestEnable(depth_test);
  cb.setDepthCompareOp(depthCompareOp);
  cb.setPolygonModeEXT(polygonMode);
  cb.setLineWidth(lineWidth);
}

void ShaderProgram::setVertexStates(vk::CommandBuffer cb) const {
  cb.setPrimitiveTopology(topology);
  cb.setVertexInputEXT({}, {});
}

void ShaderProgram::setFragmentStates(vk::CommandBuffer cb) const {
  const auto alpha_blend = vku::toVkbool((flags & kAlphaBlend) == kAlphaBlend);
  cb.setColorBlendEnableEXT(0, alpha_blend);
  cb.setColorBlendEquationEXT(0, colorBlendEquation);
}

void ShaderProgram::bindShaders(vk::CommandBuffer cb) const {
  static constexpr auto kStagesV = std::array{
      vk::ShaderStageFlagBits::eVertex,
      vk::ShaderStageFlagBits::eFragment,
  };
  auto const shaders = std::array{
      *shaders_[0],
      *shaders_[1],
  };
  cb.bindShadersEXT(kStagesV, shaders);
}

};  // namespace vkit
