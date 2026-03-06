#pragma once

#include <filesystem>
#include <ranges>

#include "vk_mem_alloc.hpp"
#include "vku/buffers/allocated_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"
#include "vku/images/bitmap_image.hpp"
#include "vku/utils/utils.hpp"
#include "vulkan/vulkan.hpp"

namespace lvk {
[[nodiscard]] constexpr auto createSamplerCreateInfo(
    vk::SamplerAddressMode wrap, vk::Filter filter,
    vk::SamplerMipmapMode mode) {
  auto ret = vk::SamplerCreateInfo{};
  ret.setAddressModeU(wrap)
      .setAddressModeV(wrap)
      .setAddressModeW(wrap)
      .setMinFilter(filter)
      .setMagFilter(filter)
      .setMaxLod(vk::LodClampNone)
      .setBorderColor(vk::BorderColor::eFloatTransparentBlack)
      .setMipmapMode(mode);
  return ret;
}

constexpr auto kLinerSamplerCiV = createSamplerCreateInfo(
    vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear,
    vk::SamplerMipmapMode::eNearest);

constexpr auto kNearestSamplerCiV = createSamplerCreateInfo(
    vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest,
    vk::SamplerMipmapMode::eLinear);

constexpr auto kNearestSamplerWithLinerMipModeCiV = createSamplerCreateInfo(
    vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest,
    vk::SamplerMipmapMode::eLinear);

struct TextureCreateInfo {
  std::string name;
  vk::Format format;
  vku::Bitmap bitmap;

  vma::Allocator allocator;

  vk::Device device;
  vk::CommandPool commandPool;
  vk::Queue queue;

  vk::SamplerCreateInfo sampler{kNearestSamplerWithLinerMipModeCiV};
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
  std::optional<vk::Sampler> sampler;

  auto descriptorInfo() const -> vk::DescriptorImageInfo {
    auto info = vk::DescriptorImageInfo{};
    info.setImageLayout(imageLayout).setImageView(*imageView);

    if (sampler.has_value()) {
      info.setSampler(*sampler);
    }

    return info;
  }

  void setSampler(vk::Sampler sampler) { this->sampler = sampler; }
};

class Texture2D : public Texture {
 public:
  Texture2D(
      void* data, vk::DeviceSize size, std::uint32_t width,
      std::uint32_t height, vma::Allocator allocator,
      const vku::DeviceCopyInfo& copyInfo, vk::Format format,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) {
    populateFromData(data, size, width, height, allocator, copyInfo, format,
                     imageUsageFlags, imageLayout);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  Texture2D(
      std::from_range_t, R&& r, std::uint32_t width, std::uint32_t height,
      vma::Allocator allocator, const vku::DeviceCopyInfo& copyInfo,
      vk::Format format,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) {
    populateFromData(r.data(), r.size() * sizeof(std::ranges::range_value_t<R>),
                     width, height, allocator, copyInfo, format,
                     imageUsageFlags, imageLayout);
  }

 private:
  void populateFromData(
      void* data, vk::DeviceSize size, std::uint32_t width,
      std::uint32_t height, vma::Allocator allocator,
      const vku::DeviceCopyInfo& copyInfo, vk::Format format,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) {
    auto mip_levels = 1;

    auto staging_buffer_ci = vk::BufferCreateInfo{};
    staging_buffer_ci.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setSize(size);

    auto staging_buffer = vku::AllocatedBuffer{allocator, staging_buffer_ci,
                                               vku::allocation::kHostWrite};
    allocator.copyMemoryToAllocation(data, staging_buffer.allocation, 0, size);

    auto buffer_copy_region = vk::BufferImageCopy2{};
    buffer_copy_region
        .setImageSubresource(vk::ImageSubresourceLayers{
            vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setImageExtent(vk::Extent3D{width, height, 1});

    auto image_ci = vk::ImageCreateInfo{};
    image_ci.setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setMipLevels(mip_levels)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setExtent(vk::Extent3D{width, height, 1})
        .setUsage(imageUsageFlags | vk::ImageUsageFlagBits::eTransferDst);

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
          .setRegions(buffer_copy_region);

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

    imageView =
        copyInfo.device.createImageViewUnique(image.getViewCreateInfo());
  }
};
};  // namespace lvk
