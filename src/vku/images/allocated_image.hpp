#pragma once

#include "image.hpp"
#include "vk_mem_alloc.hpp"

namespace vku {
struct AllocatedImage : Image {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedImage(vma::Allocator allocator,
                 const vk::ImageCreateInfo& create_info,
                 const vma::AllocationCreateInfo allocation_create_info =
                     {{}, vma::MemoryUsage::eAutoPreferDevice})
      : Image{
            .image = nullptr,
            .extent = create_info.extent,
            .format = create_info.format,
            .mip_levels = create_info.mipLevels,
            .array_layers = create_info.arrayLayers,
        }, allocator {allocator} {
    std::tie(allocation, image) =
        allocator.createImage(create_info, allocation_create_info);
  }

  AllocatedImage(const AllocatedImage&) = delete;

  AllocatedImage(AllocatedImage&& src) noexcept
      : Image{static_cast<Image>(src)},
        allocator{src.allocator},
        allocation{std::exchange(src.allocation, nullptr)} {}

  AllocatedImage& operator=(const AllocatedImage&) = delete;

  AllocatedImage& operator=(AllocatedImage&& src) noexcept {
    if (allocation) {
      allocator.destroyImage(image, allocation);
    }

    static_cast<Image&>(*this) = static_cast<Image>(src);
    allocator = src.allocator;
    allocation = std::exchange(src.allocation, nullptr);
    return *this;
  }

  virtual ~AllocatedImage() {
    if (allocation) {
      allocator.destroyImage(image, allocation);
    }
  }
};
};  // namespace vku
