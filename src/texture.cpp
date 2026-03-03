#include "texture.hpp"

#include "vku/images/bitmap_image.hpp"
#include "vku/utils/utils.hpp"

namespace lvk {

constexpr auto kWhitePixelV = std::array{std::byte{0xFF}, std::byte{0xFF},
                                         std::byte{0xFF}, std::byte{0xFF}};

constexpr auto kWhiteBitmapV = vku::Bitmap{
    .extent = vk::Extent2D{1, 1},
    .bytes = kWhitePixelV,
};

Texture::Texture(const CreateInfo& createInfo)
    : name{createInfo.name}, m_image_{createImage(createInfo)} {
  m_view_ =
      createInfo.device.createImageViewUnique(m_image_.getViewCreateInfo());
  m_sampler_ = createInfo.device.createSamplerUnique(createInfo.sampler);
}

auto Texture::descriptorInfo() const -> vk::DescriptorImageInfo {
  auto ret = vk::DescriptorImageInfo{};
  ret.setImageView(*m_view_)
      .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSampler(*m_sampler_);
  return ret;
}

auto Texture::createImage(const CreateInfo& createInfo) const
    -> vku::BitmapImage {
  auto bitmap = createInfo.bitmap;
  if (bitmap.bytes.empty() || bitmap.extent.width <= 0 ||
      createInfo.bitmap.extent.height <= 0) {
    bitmap = kWhiteBitmapV;
  }

  auto mip_levels = vku::Image::maxMipLevels(bitmap.extent);
  auto image_ci = vk::ImageCreateInfo{}
                      .setImageType(vk::ImageType::e2D)
                      .setUsage(vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled)
                      .setExtent(vku::toExtent3D(bitmap.extent))
                      .setFormat(createInfo.format)
                      .setSamples(vk::SampleCountFlagBits::e1)
                      .setMipLevels(mip_levels)
                      .setArrayLayers(1);

  auto image = vku::BitmapImage{createInfo.allocator, image_ci,
                                vku::DeviceCopyInfo{
                                    .device = createInfo.device,
                                    .commandPool = createInfo.commandPool,
                                    .queue = createInfo.queue,
                                },
                                bitmap};

  return image;
}
};  // namespace lvk
