
#pragma once

#include <cstdint>

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
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
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

};  // namespace vkit::compute::cmd
