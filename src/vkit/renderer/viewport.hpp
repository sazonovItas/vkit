#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#include "vkit/graphics/image.hpp"

namespace vkit::renderer {

struct RenderTarget {
  vk::Format format{vk::Format::eUndefined};
  vk::ImageUsageFlags usage;
  vk::ImageAspectFlags aspectFlags;

  graphics::AllocatedImage image;
  vk::UniqueImageView view;

  RenderTarget() = default;

  RenderTarget(vk::Format format, vk::ImageUsageFlags usage,
               vk::ImageAspectFlags aspect)
      : format{format}, usage{usage}, aspectFlags{aspect} {}

  [[nodiscard]] auto getWidth() const -> std::uint32_t {
    return image.extent.width;
  }

  [[nodiscard]] auto getHeight() const -> std::uint32_t {
    return image.extent.height;
  }

  void ensureSize(vk::Device device, vma::Allocator allocator,
                  std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) return;

    if (view && getWidth() == width && getHeight() == height) {
      return;
    }

    vk::ImageCreateInfo info{};
    info.setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setExtent({width, height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setUsage(usage);

    auto new_img = graphics::AllocatedImage{allocator, info,
                                            graphics::allocation::kDeviceLocal};

    vk::ImageViewCreateInfo view_info{};
    view_info.setImage(static_cast<vk::Image>(new_img))
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange({aspectFlags, 0, 1, 0, 1});

    image = std::move(new_img);
    view = device.createImageViewUnique(view_info);
  }
};

class Viewport {
 public:
  vk::Extent2D extent{0, 0};
  std::vector<RenderTarget> colorTargets;
  std::optional<RenderTarget> depthTarget;

  Viewport() = default;

  void addColorTarget(
      vk::Format format,
      vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eSampled) {
    colorTargets.emplace_back(format, usage, vk::ImageAspectFlagBits::eColor);
  }

  void setDepthTarget(vk::Format format,
                      vk::ImageUsageFlags usage =
                          vk::ImageUsageFlagBits::eDepthStencilAttachment) {
    depthTarget.emplace(format, usage, vk::ImageAspectFlagBits::eDepth);
  }

  void ensureSize(vk::Device device, vma::Allocator allocator,
                  std::uint32_t width, std::uint32_t height) {
    extent = vk::Extent2D{width, height};

    for (auto& target : colorTargets) {
      target.ensureSize(device, allocator, width, height);
    }

    if (depthTarget) {
      depthTarget->ensureSize(device, allocator, width, height);
    }
  }
};

}  // namespace vkit::renderer
