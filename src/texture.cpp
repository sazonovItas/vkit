#include "texture.hpp"

#include "vma.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {

constexpr auto kWhitePixelV = std::array{std::byte{0xFF}, std::byte{0xFF},
                                         std::byte{0xFF}, std::byte{0xFF}};

constexpr auto kWhiteBitmapV = Bitmap{.bytes = kWhitePixelV, .size = {1, 1}};

Texture::Texture(CreateInfo create_info) {
  if (create_info.bitmap.bytes.empty() || create_info.bitmap.size.x <= 0 ||
      create_info.bitmap.size.y <= 0) {
    create_info.bitmap = kWhiteBitmapV;
  }

  auto const image_ci = vma::ImageCreateInfo{
      .allocator = create_info.allocator,
      .queue_family = create_info.queue_family,
  };
  m_image_ = vma::create_sampled_image(
      image_ci, std::move(create_info.command_block), create_info.bitmap);

  auto image_view_ci = vk::ImageViewCreateInfo{};
  auto subresource_range = vk::ImageSubresourceRange{};
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1)
      .setLevelCount(m_image_.get().levels);

  image_view_ci.setImage(m_image_.get().image)
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(m_image_.get().format)
      .setSubresourceRange(subresource_range);
  m_view_ = create_info.device.createImageViewUnique(image_view_ci);

  m_sampler_ = create_info.device.createSamplerUnique(create_info.sampler);
}

auto Texture::descriptor_info() const -> vk::DescriptorImageInfo {
  auto ret = vk::DescriptorImageInfo{};
  ret.setImageView(*m_view_)
      .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSampler(*m_sampler_);
  return ret;
}

};  // namespace lvk
