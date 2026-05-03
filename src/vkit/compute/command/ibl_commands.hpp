#pragma once

#include <algorithm>
#include <cstdint>

#include "vkit/compute/pipeline_layout/ibl.hpp"
#include "vkit/graphics/command.hpp"

namespace vkit::compute::cmd {

class DispatchBrdfLutCommand final : public graphics::Command {
 public:
  DispatchBrdfLutCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                         vk::DescriptorSet set, std::uint32_t width,
                         std::uint32_t height)
      : pipeline_{pipeline},
        layout_{layout},
        set_{set},
        width_{width},
        height_{height} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout_, 0, 1, &set_,
                          0, nullptr);

    cb.dispatch(width_ / 16, height_ / 16, 1);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  std::uint32_t width_;
  std::uint32_t height_;
};

class DispatchIrradianceCommand final : public graphics::Command {
 public:
  DispatchIrradianceCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                            vk::DescriptorSet set, std::uint32_t width,
                            std::uint32_t height)
      : pipeline_{pipeline},
        layout_{layout},
        set_{set},
        width_{width},
        height_{height} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout_, 0, 1, &set_,
                          0, nullptr);

    cb.dispatch(width_ / 16, height_ / 16, 1);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  std::uint32_t width_;
  std::uint32_t height_;
};

class DispatchPrefilterCommand final : public graphics::Command {
 public:
  DispatchPrefilterCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                           vk::DescriptorSet set, std::uint32_t width,
                           std::uint32_t height, std::uint32_t layerCount)
      : pipeline_{pipeline},
        layout_{layout},
        set_{set},
        width_{width},
        height_{height},
        layerCount_{layerCount} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout_, 0, 1, &set_,
                          0, nullptr);

    for (std::uint32_t layer = 0; layer < layerCount_; ++layer) {
      float roughness = static_cast<float>(layer) /
                        static_cast<float>(std::max(1U, layerCount_ - 1));

      compute::pl::SpecularPipelineLayout::PushConstants pcs{
          .roughness = roughness,
          .layer = layer,
      };

      cb.pushConstants(layout_, vk::ShaderStageFlagBits::eCompute, 0,
                       sizeof(pcs), &pcs);

      cb.dispatch(width_ / 16, height_ / 16, 1);
    }
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  std::uint32_t width_;
  std::uint32_t height_;
  std::uint32_t layerCount_;
};

};  // namespace vkit::compute::cmd
