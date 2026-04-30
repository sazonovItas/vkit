#pragma once

#include <array>
#include <optional>
#include <vector>

#include "vkit/graphics/command.hpp"
#include "vkit/renderer/viewport.hpp"

namespace vkit::renderer::rp {

class BeginViewportPass : public graphics::Command {
 public:
  explicit BeginViewportPass(
      const Viewport& viewport, std::uint32_t colorTargetIndex = 0,
      std::optional<std::uint32_t> resolveTargetIndex = std::nullopt,
      std::array<float, 4> clearColor = {0.00F, 0.00F, 0.00F, 1.0F})
      : viewport_{viewport},
        colorIdx_{colorTargetIndex},
        resolveIdx_{resolveTargetIndex},
        clearColor_{clearColor} {}

  void record(vk::CommandBuffer cb) const override {
    std::vector<vk::ImageMemoryBarrier2> barriers;

    barriers.push_back(createBarrier(
        static_cast<vk::Image>(viewport_.colorTargets[colorIdx_].image),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite));

    if (resolveIdx_.has_value()) {
      barriers.push_back(createBarrier(
          static_cast<vk::Image>(viewport_.colorTargets[*resolveIdx_].image),
          vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          vk::AccessFlagBits2::eColorAttachmentWrite));
    }

    if (viewport_.depthTarget) {
      vk::ImageMemoryBarrier2 depth_barrier{};
      depth_barrier
          .setImage(static_cast<vk::Image>(viewport_.depthTarget->image))
          .setOldLayout(vk::ImageLayout::eUndefined)
          .setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
          .setSrcStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                           vk::PipelineStageFlagBits2::eLateFragmentTests)
          .setSrcAccessMask(vk::AccessFlagBits2::eNone)
          .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                           vk::PipelineStageFlagBits2::eLateFragmentTests)
          .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
          .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
      barriers.push_back(depth_barrier);
    }

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barriers));

    vk::RenderingAttachmentInfo color_attachment{};
    color_attachment.setImageView(*viewport_.colorTargets[colorIdx_].view)
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(resolveIdx_.has_value() ? vk::AttachmentStoreOp::eDontCare
                                            : vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue{clearColor_});

    if (resolveIdx_.has_value()) {
      color_attachment
          .setResolveImageView(*viewport_.colorTargets[*resolveIdx_].view)
          .setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
          .setResolveMode(vk::ResolveModeFlagBits::eAverage);
    }

    auto render_info = vk::RenderingInfo{}
                           .setRenderArea(vk::Rect2D{{0, 0}, viewport_.extent})
                           .setLayerCount(1)
                           .setColorAttachments(color_attachment);

    vk::RenderingAttachmentInfo depth_attachment{};
    if (viewport_.depthTarget) {
      depth_attachment.setImageView(*viewport_.depthTarget->view)
          .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setClearValue(vk::ClearDepthStencilValue{1.0F, 0});
      render_info.setPDepthAttachment(&depth_attachment);
    }

    cb.beginRendering(render_info);

    cb.setViewport(
        0,
        vk::Viewport{0.0F, 0.0F, static_cast<float>(viewport_.extent.width),
                     static_cast<float>(viewport_.extent.height), 0.0F, 1.0F});
    cb.setScissor(0, vk::Rect2D{{0, 0}, viewport_.extent});
  }

 private:
  const Viewport& viewport_;
  std::uint32_t colorIdx_;
  std::optional<std::uint32_t> resolveIdx_;
  std::array<float, 4> clearColor_;

  static auto createBarrier(vk::Image image, vk::ImageLayout oldLayout,
                            vk::ImageLayout newLayout,
                            vk::PipelineStageFlags2 dstStage,
                            vk::AccessFlags2 dstAccess)
      -> vk::ImageMemoryBarrier2 {
    return vk::ImageMemoryBarrier2{}
        .setImage(image)
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(dstStage)
        .setDstAccessMask(dstAccess)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  }
};

class EndViewportPass : public graphics::Command {
 public:
  explicit EndViewportPass(const Viewport& viewport,
                           std::uint32_t displayTargetIndex = 0)
      : viewport_{viewport}, displayIdx_{displayTargetIndex} {}

  void record(vk::CommandBuffer cb) const override {
    cb.endRendering();

    vk::ImageMemoryBarrier2 read_barrier{};
    read_barrier
        .setImage(
            static_cast<vk::Image>(viewport_.colorTargets[displayIdx_].image))
        .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    cb.pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(read_barrier));
  }

 private:
  const Viewport& viewport_;
  std::uint32_t displayIdx_;
};

};  // namespace vkit::renderer::rp
