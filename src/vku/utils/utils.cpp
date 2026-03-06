#include "vku/utils/utils.hpp"

#include "vulkan/vulkan.hpp"

namespace vku {
void generateMipmaps(vk::CommandBuffer cb, const Image& image,
                     const ImageBarrierInfo& barrierInfo) {
  auto mip_width = static_cast<int32_t>(image.extent.width);
  auto mip_height = static_cast<int32_t>(image.extent.height);

  vk::ImageMemoryBarrier2 barrier{};
  barrier.setImage(image.image)
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setSubresourceRange(vk::ImageSubresourceRange{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setLevelCount(1));

  for (uint32_t i = 1; i < image.mipLevels; i++) {
    barrier.subresourceRange.setBaseMipLevel(i - 1);
    barrier
        .setOldLayout(i == 1 ? barrierInfo.srcLayout
                             : vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    barrier.subresourceRange.setBaseMipLevel(i);

    barrier.setOldLayout(barrierInfo.srcLayout)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    vk::ImageBlit2 blit{};
    blit.setSrcSubresource(
            vk::ImageSubresourceLayers{}
                .setAspectMask(Image::inferAspectFlags(image.format))
                .setMipLevel(i - 1)
                .setBaseArrayLayer(0)
                .setLayerCount(1))
        .setSrcOffsets({
            vk::Offset3D{0, 0, 0},
            vk::Offset3D{mip_width, mip_height, 1},
        })
        .setDstSubresource(
            vk::ImageSubresourceLayers{}
                .setAspectMask(Image::inferAspectFlags(image.format))
                .setMipLevel(i)
                .setBaseArrayLayer(0)
                .setLayerCount(1))
        .setDstOffsets({
            vk::Offset3D{0, 0, 0},
            vk::Offset3D{mip_width > 1 ? mip_width / 2 : 1,
                         mip_height > 1 ? mip_height / 2 : 1, 1},
        });

    cb.blitImage2(vk::BlitImageInfo2{}
                      .setSrcImage(image.image)
                      .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                      .setDstImage(image.image)
                      .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                      .setRegions(blit)
                      .setFilter(vk::Filter::eLinear));

    barrier.subresourceRange.setBaseMipLevel(i - 1);

    barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setNewLayout(barrierInfo.dstLayout)
        .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    mip_width = mip_width > 1 ? mip_width / 2 : 1;
    mip_height = mip_height > 1 ? mip_height / 2 : 1;
  }

  barrier.subresourceRange.setBaseMipLevel(image.mipLevels - 1);

  barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
      .setNewLayout(barrierInfo.dstLayout)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
      .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
      .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
      .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);

  cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}
};  // namespace vku
