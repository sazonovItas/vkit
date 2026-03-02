#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/constants.hpp"
#include "vku/images/image.hpp"

namespace vku {
struct AllocatedImage : Image {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedImage(vma::Allocator allocator,
                 const vk::ImageCreateInfo& create_info,
                 const vma::AllocationCreateInfo& allocation_create_info =
                     vku::allocation::kDeviceLocal)
      : Image{
            .image = nullptr,
            .extent = create_info.extent,
            .format = create_info.format,
            .samples = create_info.samples,
            .mip_levels = create_info.mipLevels,
            .array_layers = create_info.arrayLayers,
        }, allocator {allocator} {
    std::tie(allocation, image) =
        allocator.createImage(create_info, allocation_create_info);
  }

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
};  // namespace vku
