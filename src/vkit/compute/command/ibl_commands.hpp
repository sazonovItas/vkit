#pragma once

#include <cstdint>

#include "vkit/compute/pipeline_layout/ibl.hpp"
#include "vkit/graphics/command.hpp"

namespace vkit::compute::cmd {

class ComputeImageBarrierCommand final : public graphics::Command {
 public:
  ComputeImageBarrierCommand(
      vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
      vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
      vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage,
      std::uint32_t baseMipLevel = 0, std::uint32_t levelCount = 1,
      std::uint32_t baseArrayLayer = 0, std::uint32_t layerCount = 1)
      : image_{image},
        oldLayout_{oldLayout},
        newLayout_{newLayout},
        srcAccess_{srcAccess},
        dstAccess_{dstAccess},
        srcStage_{srcStage},
        dstStage_{dstStage},
        baseMipLevel_{baseMipLevel},
        levelCount_{levelCount},
        baseArrayLayer_{baseArrayLayer},
        layerCount_{layerCount} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.setImage(image_)
        .setOldLayout(oldLayout_)
        .setNewLayout(newLayout_)
        .setSrcStageMask(srcStage_)
        .setSrcAccessMask(srcAccess_)
        .setDstStageMask(dstStage_)
        .setDstAccessMask(dstAccess_)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, baseMipLevel_,
                              levelCount_, baseArrayLayer_, layerCount_});

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
  }

 private:
  vk::Image image_;
  vk::ImageLayout oldLayout_;
  vk::ImageLayout newLayout_;
  vk::AccessFlags2 srcAccess_;
  vk::AccessFlags2 dstAccess_;
  vk::PipelineStageFlags2 srcStage_;
  vk::PipelineStageFlags2 dstStage_;
  std::uint32_t baseMipLevel_;
  std::uint32_t levelCount_;
  std::uint32_t baseArrayLayer_;
  std::uint32_t layerCount_;
};

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
