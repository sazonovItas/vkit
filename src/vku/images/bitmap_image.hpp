#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/constants.hpp"
#include "vku/images/allocated_image.hpp"

namespace vku {
class BitmapImage : AllocatedImage {
 public:
  struct Bitmap {
    std::span<std::byte> bytes;
    vk::Extent2D extent;
  };

  BitmapImage(vma::Allocator allocator, const vk::ImageCreateInfo& create_info,
              const vma::AllocationCreateInfo& allocation_create_info =
                  vku::allocation::kDeviceLocal)
      : AllocatedImage(allocator, create_info, allocation_create_info) {}

  BitmapImage(const BitmapImage&) = delete;
  BitmapImage& operator=(const BitmapImage&) = delete;

  BitmapImage(BitmapImage&& src) noexcept = default;

  BitmapImage& operator=(BitmapImage&& src) noexcept {
    static_cast<AllocatedImage&>(*this) =
        std::move(static_cast<AllocatedImage&>(src));
    return *this;
  }
};
};  // namespace vku
