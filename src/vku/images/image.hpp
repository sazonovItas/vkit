#pragma once

#include <ranges>

namespace vku {
struct Image {
  vk::Image image;
  vk::Extent3D extent;
  vk::Format format;
  vk::SampleCountFlagBits samples;
  std::uint32_t mip_levels;
  std::uint32_t array_layers;

  [[nodiscard]] explicit operator vk::Image() const noexcept { return image; }

  [[nodiscard]] auto get_view_create_info(
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept
      -> vk::ImageViewCreateInfo {
    return get_view_create_info(
        {
            infer_aspect_flags(format),
            0,
            mip_levels,
            0,
            array_layers,
        },
        type);
  }

  [[nodiscard]] auto get_view_create_info(
      const vk::ImageSubresourceRange &subresource_range,
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept
      -> vk::ImageViewCreateInfo {
    return {
        {}, image, type, format, {}, subresource_range,
    };
  }

  [[nodiscard]] auto get_mip_view_create_infos(
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept {
    return std::ranges::views::iota(0U, mip_levels) |
           std::views::transform(
               [this, type, aspect_flags = infer_aspect_flags(format)](
                   std::uint32_t level) {
                 auto subresource_range = vk::ImageSubresourceRange{
                     aspect_flags, level, 1, 0, vk::RemainingMipLevels,
                 };
                 return vk::ImageViewCreateInfo{
                     {}, image, type, format, {}, subresource_range};
               });
  }

  [[nodiscard]] auto mip_extent(std::uint32_t mip_level) const -> vk::Extent3D {
    return mip_extent(extent, mip_level);
  }

  [[nodiscard]] auto mip_extent_2d(std::uint32_t mip_level) const
      -> vk::Extent2D {
    return mip_extent(vk::Extent2D{extent.width, extent.height}, mip_level);
  }

  [[nodiscard]] auto subresource_range(
      vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor)
      const noexcept -> vk::ImageSubresourceRange {
    return {aspect_flags, 0, mip_levels, 0, array_layers};
  }

  [[nodiscard]] static constexpr auto infer_aspect_flags(vk::Format format)
      -> vk::ImageAspectFlags {
    switch (format) {
      case vk::Format::eUndefined:
        std::unreachable();
      case vk::Format::eD16Unorm:
      case vk::Format::eD32Sfloat:
        return vk::ImageAspectFlagBits::eDepth;
      case vk::Format::eD16UnormS8Uint:
      case vk::Format::eD24UnormS8Uint:
      case vk::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth |
               vk::ImageAspectFlagBits::eStencil;
      case vk::Format::eS8Uint:
        return vk::ImageAspectFlagBits::eStencil;
      default:
        return vk::ImageAspectFlagBits::eColor;
    }
  }

  [[nodiscard]] static constexpr auto max_mip_levels(std::uint32_t size)
      -> std::uint32_t {
    assert(size > 0U && "size must be greater than zero");
    return std::bit_width(size);
  }

  [[nodiscard]] static constexpr auto max_mip_levels(const vk::Extent2D &extent)
      -> std::uint32_t {
    return max_mip_levels(std::max(extent.width, extent.height));
  }

  [[nodiscard]] static constexpr auto max_mip_levels(const vk::Extent3D &extent)
      -> std::uint32_t {
    return max_mip_levels(
        std::max({extent.width, extent.height, extent.depth}));
  }

  [[nodiscard]] static constexpr auto mip_extent(
      const vk::Extent2D &extent, std::uint32_t mip_level) noexcept
      -> vk::Extent2D {
    assert(mip_level < max_mip_levels(extent) &&
           "mipLevel must be less than maxMipLevels(extent)");
    return {
        std::max(extent.width >> mip_level, 1U),
        std::max(extent.height >> mip_level, 1U),
    };
  }

  [[nodiscard]] static constexpr auto mip_extent(
      const vk::Extent3D &extent, std::uint32_t mip_level) noexcept
      -> vk::Extent3D {
    assert(mip_level < max_mip_levels(extent) &&
           "MipLevel must be less than maxMipLevels(extent).");
    return {
        std::max(extent.width >> mip_level, 1U),
        std::max(extent.height >> mip_level, 1U),
        std::max(extent.depth >> mip_level, 1U),
    };
  }
};
}  // namespace vku
