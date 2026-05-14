#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/graphics/image.hpp"

namespace vkit::renderer {

struct RenderTargetInfo {
  vk::Format format{vk::Format::eUndefined};
  vk::ImageUsageFlags usage;
  vk::ImageAspectFlags aspectFlags;
  vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
};

struct ViewportInfo {
  std::vector<RenderTargetInfo> colorTargets;
  std::optional<RenderTargetInfo> depthTarget;

  void addColorTarget(
      vk::Format format,
      vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eSampled,
      vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1) {
    colorTargets.push_back({
        .format = format,
        .usage = usage,
        .aspectFlags = vk::ImageAspectFlagBits::eColor,
        .samples = samples,
    });
  }

  void setDepthTarget(
      vk::Format format,
      vk::ImageUsageFlags usage =
          vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1) {
    depthTarget = {
        .format = format,
        .usage = usage,
        .aspectFlags = vk::ImageAspectFlagBits::eDepth,
        .samples = samples,
    };
  }
};

struct RenderTarget {
  vk::Format format{vk::Format::eUndefined};
  vk::ImageUsageFlags usage;
  vk::ImageAspectFlags aspectFlags;
  vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};

  graphics::AllocatedImage image;
  vk::UniqueImageView view;

  RenderTarget() = default;

  RenderTarget(vk::Format format, vk::ImageUsageFlags usage,
               vk::ImageAspectFlags aspect,
               vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
      : format{format}, usage{usage}, aspectFlags{aspect}, samples{samples} {}

  [[nodiscard]] auto getWidth() const -> std::uint32_t {
    return image.extent.width;
  }
  [[nodiscard]] auto getHeight() const -> std::uint32_t {
    return image.extent.height;
  }

  bool ensureSize(vk::Device device, vma::Allocator allocator,
                  std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) return false;
    if (view && getWidth() == width && getHeight() == height) return false;

    vk::ImageCreateInfo info{};
    info.setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setExtent({width, height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(samples)
        .setUsage(usage);

    auto new_img = graphics::AllocatedImage{allocator, info,
                                            graphics::allocation::kDeviceLocal};

    vk::ImageViewCreateInfo view_info{};
    view_info.setImage(static_cast<vk::Image>(new_img))
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange({aspectFlags, 0, 1, 0, 1});

    // Destroy view before image: VkImageView must not outlive its VkImage.
    view.reset();
    image = std::move(new_img);
    view = device.createImageViewUnique(view_info);
    return true;
  }
};

class Viewport {
 public:
  vk::Extent2D extent{0, 0};
  std::vector<RenderTarget> colorTargets;
  std::optional<RenderTarget> depthTarget;

  Viewport() = default;

  explicit Viewport(const ViewportInfo& info) {
    for (const auto& target : info.colorTargets) {
      colorTargets.emplace_back(target.format, target.usage, target.aspectFlags,
                                target.samples);
    }
    if (info.depthTarget) {
      depthTarget.emplace(info.depthTarget->format, info.depthTarget->usage,
                          info.depthTarget->aspectFlags,
                          info.depthTarget->samples);
    }
  }

  bool ensureSize(vk::Device device, vma::Allocator allocator,
                  std::uint32_t width, std::uint32_t height) {
    extent = vk::Extent2D{width, height};
    bool resized = false;
    for (auto& target : colorTargets) {
      resized |= target.ensureSize(device, allocator, width, height);
    }
    if (depthTarget) {
      resized |= depthTarget->ensureSize(device, allocator, width, height);
    }
    return resized;
  }
};

};  // namespace vkit::renderer
