#include "texture.hpp"

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/utils/utils.hpp"

namespace lvk {
void Texture2D::populate(const TextureInfo& info, vma::Allocator allocator,
                         const vku::DeviceCopyInfo& copyInfo,
                         std::uint32_t mipLevels,
                         vk::ImageUsageFlags imageUsageFlags,
                         vk::ImageLayout imageLayout,
                         vk::AccessFlagBits2 accessFlags,
                         vk::PipelineStageFlagBits2 stageFlags) {
  auto staging_buffer_ci = vk::BufferCreateInfo{};
  staging_buffer_ci.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSize(info.bytes.size());

  auto staging_buffer = vku::AllocatedBuffer{allocator, staging_buffer_ci,
                                             vku::allocation::kHostWrite};
  allocator.copyMemoryToAllocation(info.bytes.data(), staging_buffer.allocation,
                                   0, info.bytes.size());

  auto buffer_copy_region = vk::BufferImageCopy2{};
  buffer_copy_region
      .setImageSubresource(
          vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1})
      .setImageExtent(vk::Extent3D{
          static_cast<std::uint32_t>(info.width),
          static_cast<std::uint32_t>(info.height),
          1,
      });

  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(vk::ImageType::e2D)
      .setFormat(info.format)
      .setMipLevels(mipLevels)
      .setArrayLayers(1)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setExtent(vk::Extent3D{
          static_cast<std::uint32_t>(info.width),
          static_cast<std::uint32_t>(info.height),
          1,
      })
      .setUsage(imageUsageFlags | vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst);

  image = vku::AllocatedImage{allocator, image_ci};

  auto copy_buffer_to_image_fn = [&](vk::CommandBuffer cb) {
    auto subresource_range = vk::ImageSubresourceRange{};
    subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setLevelCount(mipLevels)
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

      cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
    }

    auto copy_buffer_info = vk::CopyBufferToImageInfo2{};
    copy_buffer_info.setSrcBuffer(staging_buffer.buffer)
        .setDstImage(image.image)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(buffer_copy_region);

    cb.copyBufferToImage2(copy_buffer_info);

    vku::generateMipmaps(
        cb, image,
        {
            .srcLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcAccess = vk::AccessFlagBits2::eTransferWrite,
            .srcStage = vk::PipelineStageFlagBits2::eAllCommands,
            .dstLayout = imageLayout,
            .dstAccess = accessFlags,
            .dstStage = stageFlags,
        });
  };

  vku::executeCommandAndWait(copyInfo.device, copyInfo.commandPool,
                             copyInfo.queue, copy_buffer_to_image_fn);

  this->imageLayout = imageLayout;
  imageView = copyInfo.device.createImageViewUnique(image.getViewCreateInfo());
}

void TextureCubeMap::populate(const TextureInfo& info, vma::Allocator allocator,
                              const vku::DeviceCopyInfo& copyInfo,
                              std::uint32_t mipLevels,
                              vk::ImageUsageFlags imageUsageFlags,
                              vk::ImageLayout imageLayout,
                              vk::AccessFlagBits2 accessFlags,
                              vk::PipelineStageFlagBits2 stageFlags) {
  assert(info.bytes.size() % 6 == 0 &&
         "Size should consist from 6 faces with equal size.");

  constexpr auto kArrayLayers = 6;

  auto staging_buffer_ci = vk::BufferCreateInfo{};
  staging_buffer_ci.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSize(info.bytes.size());

  auto staging_buffer = vku::AllocatedBuffer{allocator, staging_buffer_ci,
                                             vku::allocation::kHostWrite};
  allocator.copyMemoryToAllocation(info.bytes.data(), staging_buffer.allocation,
                                   0, info.bytes.size());

  auto buffer_copy_regions = std::vector<vk::BufferImageCopy2>(6);

  auto fase_size = info.bytes.size() / 6;
  auto offset = std::uint32_t{0};
  for (std::uint32_t i = 0; i < kArrayLayers; i++) {
    buffer_copy_regions[i]
        .setBufferOffset(offset)
        .setImageSubresource(vk::ImageSubresourceLayers{
            vk::ImageAspectFlagBits::eColor, 0, i, 1})
        .setImageExtent(vk::Extent3D{static_cast<std::uint32_t>(info.width),
                                     static_cast<std::uint32_t>(info.height),
                                     1});

    offset += fase_size;
  }

  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(vk::ImageType::e2D)
      .setFlags(vk::ImageCreateFlagBits::eCubeCompatible)
      .setFormat(info.format)
      .setMipLevels(mipLevels)
      .setArrayLayers(kArrayLayers)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setExtent(vk::Extent3D{static_cast<std::uint32_t>(info.width),
                              static_cast<std::uint32_t>(info.height), 1})
      .setUsage(imageUsageFlags | vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst);

  image = vku::AllocatedImage{allocator, image_ci};

  auto copy_buffer_to_image_fn = [&](vk::CommandBuffer cb) {
    auto subresource_range = vk::ImageSubresourceRange{};
    subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setLevelCount(mipLevels)
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

      cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
    }

    auto copy_buffer_info = vk::CopyBufferToImageInfo2{};
    copy_buffer_info.setSrcBuffer(staging_buffer.buffer)
        .setDstImage(image.image)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(buffer_copy_regions);

    cb.copyBufferToImage2(copy_buffer_info);

    for (std::uint32_t i = 0; i < kArrayLayers; i++) {
      vku::generateMipmaps(
          cb, image,
          {
              .srcLayout = vk::ImageLayout::eTransferDstOptimal,
              .srcAccess = vk::AccessFlagBits2::eTransferWrite,
              .srcStage = vk::PipelineStageFlagBits2::eAllCommands,
              .dstLayout = imageLayout,
              .dstAccess = accessFlags,
              .dstStage = stageFlags,
          },
          i);
    }
  };

  vku::executeCommandAndWait(copyInfo.device, copyInfo.commandPool,
                             copyInfo.queue, copy_buffer_to_image_fn);

  this->imageLayout = imageLayout;
  imageView = copyInfo.device.createImageViewUnique(
      image.getViewCreateInfo(vk::ImageViewType::eCube));
}
};  // namespace lvk
