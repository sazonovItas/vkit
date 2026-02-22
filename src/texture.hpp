#pragma once

#include <vk_mem_alloc.h>

#include "vulkan/command_block.hpp"
#include "vulkan/vma/image.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {
[[nodiscard]] constexpr auto create_sampler_ci(
    vk::SamplerAddressMode const wrap, vk::Filter const filter) {
  auto ret = vk::SamplerCreateInfo{};
  ret.setAddressModeU(wrap)
      .setAddressModeV(wrap)
      .setAddressModeW(wrap)
      .setMinFilter(filter)
      .setMagFilter(filter)
      .setMaxLod(VK_LOD_CLAMP_NONE)
      .setBorderColor(vk::BorderColor::eFloatTransparentBlack)
      .setMipmapMode(vk::SamplerMipmapMode::eNearest);
  return ret;
}

constexpr auto kSamplerCiV = create_sampler_ci(
    vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear);

struct TextureCreateInfo {
  std::string name;

  vk::Device device;
  VmaAllocator allocator;
  std::uint32_t queue_family;
  vkit::vulkan::CommandBlock command_block;
  vkit::vulkan::vma::Bitmap bitmap;

  vk::SamplerCreateInfo sampler{kSamplerCiV};
};

class Texture {
 public:
  using CreateInfo = TextureCreateInfo;

  explicit Texture(CreateInfo create_info);

  [[nodiscard]] auto descriptor_info() const -> vk::DescriptorImageInfo;

  std::string name;

 private:
  vkit::vulkan::vma::Image m_image_;
  vk::UniqueImageView m_view_;
  vk::UniqueSampler m_sampler_;
};
};  // namespace lvk
