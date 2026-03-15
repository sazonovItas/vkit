#include "compute_shader_program.hpp"

#include <array>
#include <stdexcept>

namespace vkit {
ComputeShaderProgram::ComputeShaderProgram(const CreateInfo& createInfo) {
  auto create_shader_ci =
      vk::ShaderCreateInfoEXT{}
          .setCodeSize(createInfo.computeSpirv.size_bytes())
          .setPCode(createInfo.computeSpirv.data())
          .setSetLayouts(createInfo.setLayouts)
          .setPushConstantRanges(createInfo.pushConstantRanges)
          .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
          .setPName("main")
          .setStage(
              vk::ShaderStageFlagBits::eCompute);  // Explicitly set to Compute

  auto shader_cis = std::array{create_shader_ci};

  auto result = createInfo.device.createShadersEXTUnique(shader_cis);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to create Compute Shader Object"};
  }

  shader_ = std::move(result.value[0]);
  waiter_ = createInfo.device;
}

void ComputeShaderProgram::bind(vk::CommandBuffer cb) const {
  auto const stage = vk::ShaderStageFlagBits::eCompute;
  auto const shader = *shader_;

  cb.bindShadersEXT(stage, shader);
}

void ComputeShaderProgram::dispatch(vk::CommandBuffer cb,
                                    std::uint32_t groupCountX,
                                    std::uint32_t groupCountY,
                                    std::uint32_t groupCountZ) const {
  cb.dispatch(groupCountX, groupCountY, groupCountZ);
}
};  // namespace vkit
