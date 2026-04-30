#pragma once

#include <cstdint>

#include "vkit/graphics/command.hpp"

namespace vkit::compute::rp {

class BeginImageComputePass : public graphics::Command {
 public:
  explicit BeginImageComputePass(
      vk::Image image, vk::ImageLayout oldLayout = vk::ImageLayout::eUndefined,
      std::uint32_t baseMipLevel = 0, std::uint32_t levelCount = 1,
      std::uint32_t baseArrayLayer = 0, std::uint32_t layerCount = 1)
      : image_{image},
        oldLayout_{oldLayout},
        baseMipLevel_{baseMipLevel},
        levelCount_{levelCount},
        baseArrayLayer_{baseArrayLayer},
        layerCount_{layerCount} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.setImage(image_)
        .setOldLayout(oldLayout_)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, baseMipLevel_,
                              levelCount_, baseArrayLayer_, layerCount_});

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
  }

 private:
  vk::Image image_;
  vk::ImageLayout oldLayout_;
  std::uint32_t baseMipLevel_;
  std::uint32_t levelCount_;
  std::uint32_t baseArrayLayer_;
  std::uint32_t layerCount_;
};

class EndImageComputePass : public graphics::Command {
 public:
  explicit EndImageComputePass(
      vk::Image image,
      vk::ImageLayout newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      std::uint32_t baseMipLevel = 0, std::uint32_t levelCount = 1,
      std::uint32_t baseArrayLayer = 0, std::uint32_t layerCount = 1)
      : image_{image},
        newLayout_{newLayout},
        baseMipLevel_{baseMipLevel},
        levelCount_{levelCount},
        baseArrayLayer_{baseArrayLayer},
        layerCount_{layerCount} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.setImage(image_)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(newLayout_)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                         vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, baseMipLevel_,
                              levelCount_, baseArrayLayer_, layerCount_});

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
  }

 private:
  vk::Image image_;
  vk::ImageLayout newLayout_;
  std::uint32_t baseMipLevel_;
  std::uint32_t levelCount_;
  std::uint32_t baseArrayLayer_;
  std::uint32_t layerCount_;
};

};  // namespace vkit::compute::rp
