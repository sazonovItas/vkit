#include "vku/utils/utils.hpp"

#include <fstream>

namespace vku {
auto toSpirV(std::filesystem::path const& path) -> std::vector<std::uint32_t> {
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    throw std::runtime_error{
        std::format("failed to open file: '{}'", path.generic_string())};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  if (0 != usize % sizeof(std::uint32_t)) {
    throw std::runtime_error{std::format("invalid SPIR-V size: {}", usize)};
  }

  file.seekg({}, std::ios::beg);
  auto ret = std::vector<std::uint32_t>{};
  ret.resize(usize / sizeof(std::uint32_t));
  void* data = ret.data();
  file.read(static_cast<char*>(data), size);
  return ret;
}

void generateMipmaps(vk::CommandBuffer cb, const Image& image,
                     const ImageBarrierInfo& barrierInfo,
                     const std::uint32_t baseArrayLayer) {
  auto mip_width = static_cast<int32_t>(image.extent.width);
  auto mip_height = static_cast<int32_t>(image.extent.height);

  vk::ImageMemoryBarrier2 barrier{};
  barrier.setImage(image.image)
      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
      .setSubresourceRange(vk::ImageSubresourceRange{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(baseArrayLayer)
                               .setLayerCount(1)
                               .setLevelCount(1));

  for (uint32_t i = 1; i < image.mipLevels; i++) {
    barrier.subresourceRange.setBaseMipLevel(i - 1);
    barrier
        .setOldLayout(i == 1 ? barrierInfo.srcLayout
                             : vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setSrcStageMask(i == 1 ? barrierInfo.srcStage
                                : vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(i == 1 ? barrierInfo.srcAccess
                                 : vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    barrier.subresourceRange.setBaseMipLevel(i);

    barrier.setOldLayout(barrierInfo.srcLayout)
        .setSrcStageMask(barrierInfo.srcStage)
        .setSrcAccessMask(barrierInfo.srcAccess)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    vk::ImageBlit2 blit{};
    blit.setSrcSubresource(
            vk::ImageSubresourceLayers{}
                .setAspectMask(Image::inferAspectFlags(image.format))
                .setMipLevel(i - 1)
                .setBaseArrayLayer(baseArrayLayer)
                .setLayerCount(1))
        .setSrcOffsets({
            vk::Offset3D{0, 0, 0},
            vk::Offset3D{mip_width, mip_height, 1},
        })
        .setDstSubresource(
            vk::ImageSubresourceLayers{}
                .setAspectMask(Image::inferAspectFlags(image.format))
                .setMipLevel(i)
                .setBaseArrayLayer(baseArrayLayer)
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
        .setDstStageMask(barrierInfo.dstStage)
        .setDstAccessMask(barrierInfo.dstAccess);

    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    mip_width = mip_width > 1 ? mip_width / 2 : 1;
    mip_height = mip_height > 1 ? mip_height / 2 : 1;
  }

  barrier.subresourceRange.setBaseMipLevel(image.mipLevels - 1);

  barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
      .setNewLayout(barrierInfo.dstLayout)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
      .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
      .setDstStageMask(barrierInfo.dstStage)
      .setDstAccessMask(barrierInfo.dstAccess);

  cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}
};  // namespace vku
