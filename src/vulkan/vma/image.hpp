#pragma once

#include <vk_mem_alloc.h>

#include "util/scoped.hpp"
#include "vulkan/command_block.hpp"

namespace vkit::vulkan::vma {
struct RawImage {
  auto operator==(RawImage const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Image image;
  vk::Extent2D extent;
  vk::Format format{};
  std::uint32_t levels{};
};

struct ImageDeleter {
  void operator()(RawImage const& raw_image) const noexcept;
};

using Image = util::Scoped<RawImage, ImageDeleter>;

struct ImageCreateInfo {
  VmaAllocator allocator;
  std::uint32_t queue_family;
};

[[nodiscard]] auto create_image(ImageCreateInfo const& create_info,
                                vk::ImageUsageFlags usage, std::uint32_t levels,
                                vk::Format format, vk::Extent2D extent)
    -> Image;

struct Bitmap {
  std::span<std::byte const> bytes;
  glm::ivec2 size{};
};

[[nodiscard]] auto create_sampled_image(ImageCreateInfo const& create_info,
                                        CommandBlock command_block,
                                        Bitmap const& bitmap) -> Image;
};  // namespace vkit::vulkan::vma
