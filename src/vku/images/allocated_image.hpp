#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/constants.hpp"
#include "vku/images/image.hpp"

namespace vku {
struct AllocatedImage : Image {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedImage(vma::Allocator allocator, const vk::ImageCreateInfo& creaeInfo,
                 const vma::AllocationCreateInfo& allocationCreateInfo =
                     vku::allocation::kDeviceLocal)
      : Image{
            .image = nullptr,
            .extent = creaeInfo.extent,
            .format = creaeInfo.format,
            .samples = creaeInfo.samples,
            .mipLevels = creaeInfo.mipLevels,
            .arrayLayers = creaeInfo.arrayLayers,
        },
        allocator{allocator} {
    std::tie(allocation, image) =
        allocator.createImage(creaeInfo, allocationCreateInfo);
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
};  // namespace vku
