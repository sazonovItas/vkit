#pragma once

#include <ranges>
#include <vk_mem_alloc.hpp>

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/graphics/allocation.hpp"

namespace vkit::graphics {

struct Image {
  vk::Image image{nullptr};
  vk::ImageType imageType{vk::ImageType::e2D};
  vk::Extent3D extent{};
  dataformat::Format format{dataformat::Format::eUndefined};
  vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
  std::uint32_t mipLevels{0};
  std::uint32_t arrayLayers{0};

  [[nodiscard]] explicit operator vk::Image() const noexcept { return image; }

  [[nodiscard]] auto getViewCreateInfo(
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept
      -> vk::ImageViewCreateInfo {
    return getViewCreateInfo(
        {
            inferAspectFlags(format),
            0,
            mipLevels,
            0,
            arrayLayers,
        },
        type);
  }

  [[nodiscard]] auto getViewCreateInfo(
      const vk::ImageSubresourceRange& subresource_range,
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept
      -> vk::ImageViewCreateInfo {
    return {
        {}, image, type, format, {}, subresource_range,
    };
  }

  [[nodiscard]] auto getMipViewCreateInfos(
      vk::ImageViewType type = vk::ImageViewType::e2D) const noexcept {
    return std::ranges::views::iota(0U, mipLevels) |
           std::views::transform(
               [this, type,
                aspect_flags = inferAspectFlags(format)](std::uint32_t level) {
                 auto subresource_range = vk::ImageSubresourceRange{
                     aspect_flags, level, 1, 0, vk::RemainingArrayLayers,
                 };
                 return vk::ImageViewCreateInfo{
                     {}, image, type, format, {}, subresource_range};
               });
  }

  [[nodiscard]] auto mipExtent(std::uint32_t mip_level) const -> vk::Extent3D {
    return mipExtent(extent, mip_level);
  }

  [[nodiscard]] auto mipExtent2D(std::uint32_t mip_level) const
      -> vk::Extent2D {
    return mipExtent(vk::Extent2D{extent.width, extent.height}, mip_level);
  }

  [[nodiscard]] auto subresourceRange() const noexcept
      -> vk::ImageSubresourceRange {
    return {inferAspectFlags(format), 0, mipLevels, 0, arrayLayers};
  }

  [[nodiscard]] auto getBaseMipSizeBytes() const -> std::size_t {
    return extent.width * extent.height * extent.depth *
           dataformat::getPixelByteSize(format) * arrayLayers;
  }

  [[nodiscard]] static constexpr auto inferAspectFlags(
      dataformat::Format format) -> vk::ImageAspectFlags {
    switch (format) {
      case dataformat::Format::eUndefined:
        std::unreachable();
      case dataformat::Format::eD16Unorm:
      case dataformat::Format::eD32Sfloat:
        return vk::ImageAspectFlagBits::eDepth;
      case dataformat::Format::eD16UnormS8Uint:
      case dataformat::Format::eD24UnormS8Uint:
      case dataformat::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth |
               vk::ImageAspectFlagBits::eStencil;
      case dataformat::Format::eS8Uint:
        return vk::ImageAspectFlagBits::eStencil;
      default:
        return vk::ImageAspectFlagBits::eColor;
    }
  }

  [[nodiscard]] static constexpr auto maxMipLevels(std::uint32_t size)
      -> std::uint32_t {
    assert(size > 0U && "size must be greater than zero");
    return std::bit_width(size);
  }

  [[nodiscard]] static constexpr auto maxMipLevels(const vk::Extent2D& extent)
      -> std::uint32_t {
    return maxMipLevels(std::max(extent.width, extent.height));
  }

  [[nodiscard]] static constexpr auto maxMipLevels(const vk::Extent3D& extent)
      -> std::uint32_t {
    return maxMipLevels(std::max({extent.width, extent.height, extent.depth}));
  }

  [[nodiscard]] static constexpr auto mipExtent(
      const vk::Extent2D& extent, std::uint32_t mip_level) noexcept
      -> vk::Extent2D {
    assert(mip_level < maxMipLevels(extent) &&
           "mipLevel must be less than maxMipLevels(extent)");
    return {
        std::max(extent.width >> mip_level, 1U),
        std::max(extent.height >> mip_level, 1U),
    };
  }

  [[nodiscard]] static constexpr auto mipExtent(
      const vk::Extent3D& extent, std::uint32_t mip_level) noexcept
      -> vk::Extent3D {
    assert(mip_level < maxMipLevels(extent) &&
           "MipLevel must be less than maxMipLevels(extent).");
    return {
        std::max(extent.width >> mip_level, 1U),
        std::max(extent.height >> mip_level, 1U),
        std::max(extent.depth >> mip_level, 1U),
    };
  }
};

struct AllocatedImage : Image {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedImage(vma::Allocator allocator,
                 const vk::ImageCreateInfo& createInfo,
                 const vma::AllocationCreateInfo& allocationCreateInfo =
                     allocation::kDeviceLocal)
      : Image{
            .image = nullptr,
            .imageType = createInfo.imageType,
            .extent = createInfo.extent,
            .format = createInfo.format,
            .samples = createInfo.samples,
            .mipLevels = createInfo.mipLevels,
            .arrayLayers = createInfo.arrayLayers,
        },
        allocator{allocator} {
    std::tie(allocation, image) =
        allocator.createImage(createInfo, allocationCreateInfo);
  }

  AllocatedImage() = default;
  AllocatedImage(const AllocatedImage&) = delete;
  AllocatedImage& operator=(const AllocatedImage&) = delete;

  AllocatedImage(AllocatedImage&& src) noexcept
      : Image{static_cast<Image>(src)},
        allocator{src.allocator},
        allocation{std::exchange(src.allocation, nullptr)} {
    image = std::exchange(src.image, nullptr);
  }

  AllocatedImage& operator=(AllocatedImage&& src) noexcept {
    if (allocation) {
      allocator.destroyImage(image, allocation);
    }

    static_cast<Image&>(*this) = static_cast<Image>(src);
    allocator = src.allocator;
    allocation = std::exchange(src.allocation, nullptr);
    image = std::exchange(src.image, nullptr);
    return *this;
  }

  virtual ~AllocatedImage() {
    if (allocation) {
      allocator.destroyImage(image, allocation);
    }
  }
};

}  // namespace vkit::graphics
