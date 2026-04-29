#pragma once

namespace vkit::graphics {

struct PipelineLayout : public vk::UniquePipelineLayout {
  explicit PipelineLayout(vk::Device device,
                          const vk::PipelineLayoutCreateInfo& createInfo)
      : vk::UniquePipelineLayout{
            device.createPipelineLayoutUnique(createInfo),
        } {}
};

};  // namespace vkit::graphics
