#pragma once

namespace vkit {
static constexpr auto kSampleCount = vk::SampleCountFlagBits::e8;

struct RenderTarget {
  vk::Image swapchainImage;
  vk::ImageView swapchainImageView;
  vk::Image colorImage;
  vk::ImageView colorImageView;
  vk::Image depthImage;
  vk::ImageView depthImageView;
  vk::Extent2D extent;
};
};  // namespace vkit
