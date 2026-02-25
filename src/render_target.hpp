#pragma once

namespace lvk {
static constexpr auto kSampleCount = vk::SampleCountFlagBits::e8;

struct RenderTarget {
  vk::Image image;
  vk::ImageView image_view;
  vk::Image color_image;
  vk::ImageView color_image_view;
  vk::Image depth_image;
  vk::ImageView depth_image_view;
  vk::Extent2D extent;
};
}  // namespace lvk
