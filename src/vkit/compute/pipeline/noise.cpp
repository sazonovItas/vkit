#include "vkit/compute/pipeline/noise.hpp"

#include "vkit/graphics/shader_module.hpp"

namespace vkit::compute {

NoisePipeline::NoisePipeline(vk::Device device,
                             const pl::NoisePipelineLayout& layout,
                             const std::filesystem::path& shader_spv) {
  graphics::SpirVShaderModule shader(device, shader_spv);
  auto stage = shader.stageCreateInfo(vk::ShaderStageFlagBits::eCompute);

  auto [result, pipe] = device.createComputePipelineUnique(
      nullptr, vk::ComputePipelineCreateInfo{{}, stage, layout.get()});
  pipeline_ = std::move(pipe);
}

void NoisePipeline::recordDispatch(
    vk::CommandBuffer cb, vk::PipelineLayout layout, vk::DescriptorSet ds,
    const core::events::NoisePushConstants& pc) const {
  cb.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline_);
  cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, ds, {});
  cb.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc),
                   &pc);

  uint32_t gx = (pc.width + 15) / 16;
  uint32_t gy = (pc.height + 15) / 16;
  cb.dispatch(gx, gy, 1);
}

};  // namespace vkit::compute
