#pragma once

#include <array>

#include "vkit/graphics/command.hpp"
#include "vkit/renderer/viewport.hpp"

namespace vkit::renderer::rp {

class BeginViewportPass : public graphics::Command {
 public:
  explicit BeginViewportPass(const Viewport& viewport,
                             std::array<float, 4> clearColor = {0.05F, 0.05F,
                                                                0.05F, 1.0F})
      : viewport_{viewport}, clearColor_{clearColor} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 color_barrier{};
    color_barrier
        .setImage(static_cast<vk::Image>(viewport_.colorTargets[0].image))
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    std::vector<vk::ImageMemoryBarrier2> barriers = {color_barrier};

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
    color_attachment.setImageView(*viewport_.colorTargets[0].view)
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue{clearColor_});

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
  std::array<float, 4> clearColor_;
};

class EndViewportPass : public graphics::Command {
 public:
  explicit EndViewportPass(const Viewport& viewport) : viewport_{viewport} {}

  void record(vk::CommandBuffer cb) const override {
    cb.endRendering();

    vk::ImageMemoryBarrier2 read_barrier{};
    read_barrier
        .setImage(static_cast<vk::Image>(viewport_.colorTargets[0].image))
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
};

};  // namespace vkit::renderer::rp
