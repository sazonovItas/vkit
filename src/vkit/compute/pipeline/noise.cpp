#include "vkit/compute/pipeline/noise.hpp"

#include "vkit/graphics/shader_module.hpp"

namespace vkit::compute {

NoisePipeline::NoisePipeline(vk::Device device,
                             const std::filesystem::path& shader_spv) {
  std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
      vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageImage, 1,
                                     vk::ShaderStageFlagBits::eCompute},
      vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageImage, 1,
                                     vk::ShaderStageFlagBits::eCompute},
  };
  dsl_ = device.createDescriptorSetLayoutUnique(
      vk::DescriptorSetLayoutCreateInfo{{}, bindings});

  vk::PushConstantRange pc_range{vk::ShaderStageFlagBits::eCompute, 0,
                                 sizeof(core::events::NoisePushConstants)};
  layout_ = device.createPipelineLayoutUnique(
      vk::PipelineLayoutCreateInfo{{}, *dsl_, pc_range});

  graphics::SpirVShaderModule shader(device, shader_spv);
  auto stage = shader.stageCreateInfo(vk::ShaderStageFlagBits::eCompute);

  auto [result, pipe] = device.createComputePipelineUnique(
      nullptr, vk::ComputePipelineCreateInfo{{}, stage, *layout_});
  pipeline_ = std::move(pipe);
}

void NoisePipeline::recordDispatch(
    vk::CommandBuffer cb, vk::DescriptorSet ds,
    const core::events::NoisePushConstants& pc) const {
  cb.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline_);
  cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout_, 0, ds, {});
  cb.pushConstants(*layout_, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc),
                   &pc);
  uint32_t gx = (pc.width + 15) / 16;
  uint32_t gy = (pc.height + 15) / 16;
  cb.dispatch(gx, gy, 1);
}

};  // namespace vkit::compute
