#pragma once

#include <filesystem>
#include <ranges>

#include "gli/gli.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/buffers/allocated_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"
#include "vku/images/bitmap_image.hpp"
#include "vku/utils/utils.hpp"

namespace lvk {
[[nodiscard]] constexpr auto createSamplerCreateInfo(
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

constexpr auto kSamplerCiV = createSamplerCreateInfo(
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

class TextureLVK {
 public:
  using CreateInfo = TextureCreateInfo;

  explicit TextureLVK(const CreateInfo& createInfo);

  [[nodiscard]] auto descriptorInfo() const -> vk::DescriptorImageInfo;

  std::string name;

 private:
  vku::BitmapImage m_image_;
  vk::UniqueImageView m_view_;
  vk::UniqueSampler m_sampler_;

  auto createImage(const CreateInfo& createInfo) const -> vku::BitmapImage;
};

struct Texture {
  vk::ImageLayout imageLayout;
  vku::AllocatedImage image;
  vk::UniqueImageView imageView;
  std::optional<vk::UniqueSampler> sampler;

  auto descriptorInfo() const -> vk::DescriptorImageInfo {
    auto info = vk::DescriptorImageInfo{};
    info.setImageLayout(imageLayout).setImageView(*imageView);

    if (sampler.has_value()) {
      info.setSampler(sampler->get());
    }

    return info;
  }
};

class Texture2D : public Texture {
 public:
  Texture2D(
      const std::filesystem::path& path, vma::Allocator allocator,
      const vku::DeviceCopyInfo& copyInfo, vk::Format format,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) {
    gli::texture2d texture(gli::load(path.string()));
    if (texture.empty()) {
      throw std::runtime_error{
          std::format("failed to load texture from path: `{}`", path.string())};
    }

    auto width = static_cast<std::uint32_t>(texture[0].extent().x);
    auto height = static_cast<std::uint32_t>(texture[0].extent().y);
    auto mip_levels = static_cast<std::uint32_t>(texture.levels());

    auto staging_buffer_ci = vk::BufferCreateInfo{};
    staging_buffer_ci.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setSize(texture.size());

    auto staging_buffer = vku::AllocatedBuffer{allocator, staging_buffer_ci,
                                               vku::allocation::kHostWrite};
    allocator.copyMemoryToAllocation(texture.data(), staging_buffer.allocation,
                                     0, texture.size());

    auto offset = std::uint32_t{0};
    auto buffer_copy_regions = std::vector<vk::BufferImageCopy2>(mip_levels);

    for (std::uint32_t i = 0; i < mip_levels; i++) {
      buffer_copy_regions[i]
          .setImageSubresource(vk::ImageSubresourceLayers{
              vk::ImageAspectFlagBits::eColor, i, 0, 1})
          .setImageExtent(vk::Extent3D{
              static_cast<std::uint32_t>(texture[i].extent().x),
              static_cast<std::uint32_t>(texture[i].extent().y),
              1,
          })
          .setBufferOffset(offset);

      offset += static_cast<std::uint32_t>(texture[i].size());
    }

    auto image_ci = vk::ImageCreateInfo{};
    image_ci.setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setMipLevels(mip_levels)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setExtent(vk::Extent3D{width, height, 1})
        .setUsage(imageUsageFlags | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst);

    image = vku::AllocatedImage{allocator, image_ci};

    auto copy_buffer_to_image_fn = [&](vk::CommandBuffer cb) {
      auto subresource_range = vk::ImageSubresourceRange{};
      subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
          .setLevelCount(mip_levels)
          .setLayerCount(1);

      {
        auto barrier = vk::ImageMemoryBarrier2{};
        barrier.setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setImage(image.image)
            .setSubresourceRange(subresource_range);

        cb.pipelineBarrier2(
            vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
      }

      auto copy_buffer_info = vk::CopyBufferToImageInfo2{};
      copy_buffer_info.setSrcBuffer(staging_buffer.buffer)
          .setDstImage(image.image)
          .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
          .setRegions(buffer_copy_regions);

      cb.copyBufferToImage2(copy_buffer_info);

      this->imageLayout = imageLayout;

      {
        auto barrier = vk::ImageMemoryBarrier2{};
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(imageLayout)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setImage(image.image)
            .setSubresourceRange(subresource_range);

        cb.pipelineBarrier2(
            vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
      }
    };

    vku::executeCommandAndWait(copyInfo.device, copyInfo.commandPool,
                               copyInfo.queue, copy_buffer_to_image_fn);

    auto sampler_ci = createSamplerCreateInfo(vk::SamplerAddressMode::eRepeat,
                                              vk::Filter::eLinear);

    sampler = copyInfo.device.createSamplerUnique(sampler_ci);
    imageView =
        copyInfo.device.createImageViewUnique(image.getViewCreateInfo());
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  Texture2D(
      std::from_range_t, R&& r, std::uint32_t width, std::uint32_t height,
      const vku::DeviceCopyInfo& copyInfo, vk::Format format,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal){};
};
};  // namespace lvk
