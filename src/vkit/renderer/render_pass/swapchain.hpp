#pragma once

#include <array>

#include "vkit/graphics/command.hpp"

namespace vkit::renderer::rp {

class BeginSwapchainPass : public graphics::Command {
 public:
  BeginSwapchainPass(vk::Image swapchainImage, vk::ImageView swapchainView,
                     vk::Extent2D extent,
                     std::array<float, 4> clearColor = {0.0F, 0.0F, 0.0F, 1.0F})
      : image_{swapchainImage},
        view_{swapchainView},
        extent_{extent},
        clearColor_{clearColor} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 color_barrier{};
    color_barrier.setImage(image_)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    cb.pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(color_barrier));

    vk::RenderingAttachmentInfo color_attachment{};
    color_attachment.setImageView(view_)
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue{clearColor_});

    auto render_info = vk::RenderingInfo{}
                           .setRenderArea(vk::Rect2D{{0, 0}, extent_})
                           .setLayerCount(1)
                           .setColorAttachments(color_attachment);

    cb.beginRendering(render_info);

    cb.setViewport(
        0, vk::Viewport{0.0F, 0.0F, static_cast<float>(extent_.width),
                        static_cast<float>(extent_.height), 0.0F, 1.0F});
    cb.setScissor(0, vk::Rect2D{{0, 0}, extent_});
  }

 private:
  vk::Image image_;
  vk::ImageView view_;
  vk::Extent2D extent_;
  std::array<float, 4> clearColor_;
};

class EndSwapchainPass : public graphics::Command {
 public:
  explicit EndSwapchainPass(vk::Image swapchainImage)
      : image_{swapchainImage} {}

  void record(vk::CommandBuffer cb) const override {
    cb.endRendering();

    vk::ImageMemoryBarrier2 present_barrier{};
    present_barrier.setImage(image_)
        .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    cb.pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(present_barrier));
  }

 private:
  vk::Image image_;
};

};  // namespace vkit::renderer::rp
