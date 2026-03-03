#include "texture.hpp"

#include "vku/utils/utils.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {

constexpr auto kWhitePixelV = std::array{std::byte{0xFF}, std::byte{0xFF},
                                         std::byte{0xFF}, std::byte{0xFF}};

constexpr auto kWhiteBitmapV = vku::Bitmap{
    .extent = vk::Extent2D{1, 1},
    .bytes = kWhitePixelV,
};

Texture::Texture(CreateInfo createInfo) : name{createInfo.name} {
  if (createInfo.bitmap.bytes.empty() || createInfo.bitmap.extent.width <= 0 ||
      createInfo.bitmap.extent.height <= 0) {
    createInfo.bitmap = kWhiteBitmapV;
  }

  auto mip_levels = vku::Image::maxMipLevels(createInfo.bitmap.extent);
  auto image_ci = vk::ImageCreateInfo{}
                      .setImageType(vk::ImageType::e2D)
                      .setUsage(vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled)
                      .setExtent(vku::toExtent3D(createInfo.bitmap.extent))
                      .setFormat(createInfo.format)
                      .setSamples(vk::SampleCountFlagBits::e1)
                      .setMipLevels(mip_levels)
                      .setArrayLayers(1);

  m_image_.emplace(createInfo.allocator, image_ci);
  m_image_->update(createInfo.bitmap, createInfo.device, createInfo.commandPool,
                   createInfo.queue);

  m_view_ =
      createInfo.device.createImageViewUnique(m_image_->getViewCreateInfo());
  m_sampler_ = createInfo.device.createSamplerUnique(createInfo.sampler);
}

auto Texture::descriptorInfo() const -> vk::DescriptorImageInfo {
  auto ret = vk::DescriptorImageInfo{};
  ret.setImageView(*m_view_)
      .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSampler(*m_sampler_);
  return ret;
}
};  // namespace lvk
