#pragma once

#include "vkit/graphics/command.hpp"
#include "vkit/graphics/image.hpp"

namespace vkit::renderer::rp {

class BeginShadowPass : public graphics::Command {
 public:
  BeginShadowPass(vk::Image depthImage, vk::ImageView depthView,
                  vk::Extent2D extent)
      : depthImage_{depthImage}, depthView_{depthView}, extent_{extent} {}

  void record(vk::CommandBuffer cb) const override {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.setImage(depthImage_)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                         vk::PipelineStageFlagBits2::eLateFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
        .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    vk::RenderingAttachmentInfo depth_attachment{};
    depth_attachment.setImageView(depthView_)
        .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearDepthStencilValue{1.0F, 0});

    auto render_info =
        vk::RenderingInfo{}
            .setRenderArea(vk::Rect2D{{0, 0}, extent_})
            .setLayerCount(1)
            .setPDepthAttachment(&depth_attachment);

    cb.beginRendering(render_info);
    cb.setViewport(0,
                   vk::Viewport{0.0F, 0.0F,
                                static_cast<float>(extent_.width),
                                static_cast<float>(extent_.height),
                                0.0F, 1.0F});
    cb.setScissor(0, vk::Rect2D{{0, 0}, extent_});
  }

 private:
  vk::Image depthImage_;
  vk::ImageView depthView_;
  vk::Extent2D extent_;
};

class EndShadowPass : public graphics::Command {
 public:
  explicit EndShadowPass(vk::Image depthImage) : depthImage_{depthImage} {}

  void record(vk::CommandBuffer cb) const override {
    cb.endRendering();

    vk::ImageMemoryBarrier2 barrier{};
    barrier.setImage(depthImage_)
        .setOldLayout(vk::ImageLayout::eDepthAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eLateFragmentTests)
        .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
  }

 private:
  vk::Image depthImage_;
};

};  // namespace vkit::renderer::rp
