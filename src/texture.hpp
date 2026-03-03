#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/images/bitmap_image.hpp"

namespace lvk {
[[nodiscard]] constexpr auto create_sampler_ci(
    vk::SamplerAddressMode const wrap, vk::Filter const filter) {
  auto ret = vk::SamplerCreateInfo{};
  ret.setAddressModeU(wrap)
      .setAddressModeV(wrap)
      .setAddressModeW(wrap)
      .setMinFilter(filter)
      .setMagFilter(filter)
      .setMaxLod(vk::LodClampNone)
      .setBorderColor(vk::BorderColor::eFloatTransparentBlack)
      .setMipmapMode(vk::SamplerMipmapMode::eNearest);
  return ret;
}

constexpr auto kSamplerCiV = create_sampler_ci(
    vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear);

struct TextureCreateInfo {
  std::string name;
  vk::Format format;
  vku::Bitmap bitmap;

  vma::Allocator allocator;

  vk::Device device;
  vk::CommandPool commandPool;
  vk::Queue queue;

  vk::SamplerCreateInfo sampler{kSamplerCiV};
};

class Texture {
 public:
  using CreateInfo = TextureCreateInfo;

  explicit Texture(CreateInfo createInfo);

  [[nodiscard]] auto descriptorInfo() const -> vk::DescriptorImageInfo;

  std::string name;

 private:
  std::optional<vku::BitmapImage> m_image_;
  vk::UniqueImageView m_view_;
  vk::UniqueSampler m_sampler_;
};
};  // namespace lvk
