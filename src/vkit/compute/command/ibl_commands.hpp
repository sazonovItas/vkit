#pragma once

#include <cstdint>

#include "vkit/compute/pipeline_layout/ibl.hpp"
#include "vkit/graphics/command.hpp"

namespace vkit::compute::cmd {

class GenerateBrdfLutCommand final : public graphics::Command {
 public:
  GenerateBrdfLutCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
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
    cb.dispatch((width_ + 15) / 16, (height_ + 15) / 16, 1);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  std::uint32_t width_;
  std::uint32_t height_;
};

class GenerateIrradianceCommand final : public graphics::Command {
 public:
  GenerateIrradianceCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
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
    cb.dispatch((width_ + 15) / 16, (height_ + 15) / 16, 1);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  std::uint32_t width_;
  std::uint32_t height_;
};

class GeneratePrefilterCommand final : public graphics::Command {
 public:
  GeneratePrefilterCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                           vk::DescriptorSet set, std::uint32_t width,
                           std::uint32_t height, std::uint32_t layers)
      : pipeline_{pipeline},
        layout_{layout},
        set_{set},
        width_{width},
        height_{height},
        layerCount_{layers} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout_, 0, 1, &set_,
                          0, nullptr);

    for (std::uint32_t layer = 0; layer < layerCount_; ++layer) {
      float roughness =
          (layerCount_ > 1)
              ? static_cast<float>(layer) / static_cast<float>(layerCount_ - 1)
              : 0.0F;

      pl::SpecularPipelineLayout::PushConstants pcs{
          .roughness = roughness,
          .layer = layer,
      };

      cb.pushConstants(layout_, vk::ShaderStageFlagBits::eCompute, 0,
                       sizeof(pl::SpecularPipelineLayout::PushConstants), &pcs);

      cb.dispatch((width_ + 15) / 16, (height_ + 15) / 16, 1);
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
