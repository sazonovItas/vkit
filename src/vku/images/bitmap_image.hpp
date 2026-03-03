#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/buffers/mapped_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"
#include "vku/images/allocated_image.hpp"
#include "vku/utils/utils.hpp"

namespace vku {
struct Bitmap {
  vk::Extent2D extent;
  std::span<const std::byte> bytes;
};

class BitmapImage : public AllocatedImage {
 public:
  BitmapImage(vma::Allocator allocator, const vk::ImageCreateInfo& createInfo,
              const vma::AllocationCreateInfo& allocationCreateInfo =
                  vku::allocation::kDeviceLocal)
      : AllocatedImage(allocator, createInfo, allocationCreateInfo) {}

  BitmapImage(const BitmapImage&) = delete;
  BitmapImage& operator=(const BitmapImage&) = delete;

  BitmapImage(BitmapImage&& src) noexcept = default;

  BitmapImage& operator=(BitmapImage&& src) noexcept {
    static_cast<AllocatedImage&>(*this) =
        std::move(static_cast<AllocatedImage&>(src));
    return *this;
  }

  void update(const Bitmap& bitmap, vk::Device device,
              vk::CommandPool commandPool, vk::Queue queue,
              const ImageBarrierInfo& barrierInfo = {
                  .srcLayout = vk::ImageLayout::eUndefined,
                  .srcAccess = vk::AccessFlagBits2::eNone,
                  .srcStage = vk::PipelineStageFlagBits2::eTopOfPipe,
                  .dstLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                  .dstAccess = vk::AccessFlagBits2::eShaderRead,
                  .dstStage = vk::PipelineStageFlagBits2::eFragmentShader,
              }) {
    assert(bitmap.extent.width == extent.width &&
           bitmap.extent.height == extent.height &&
           "Bitmap extent should be equal to image extent.");

    auto staging_buffer = MappedBuffer{
        allocator,
        std::from_range_t{},
        bitmap.bytes,
        vk::BufferUsageFlagBits::eTransferSrc,
    };

    auto k_copy_and_mip = [&](vk::CommandBuffer cb) {
      auto barrier = vk::ImageMemoryBarrier2{}
                         .setImage(image)
                         .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                         .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                         .setOldLayout(barrierInfo.srcLayout)
                         .setSrcStageMask(barrierInfo.srcStage)
                         .setSrcAccessMask(barrierInfo.srcAccess)
                         .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                         .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                         .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
                         .setSubresourceRange(this->subresourceRange());

      cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

      auto copy_region =
          vk::BufferImageCopy2{}
              .setImageSubresource(vk::ImageSubresourceLayers{}
                                       .setAspectMask(inferAspectFlags(format))
                                       .setLayerCount(1)
                                       .setMipLevel(0))
              .setImageExtent(extent);

      cb.copyBufferToImage2(
          vk::CopyBufferToImageInfo2{}
              .setDstImage(image)
              .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
              .setSrcBuffer(staging_buffer.buffer)
              .setRegions(copy_region));

      auto mip_barrier_info = ImageBarrierInfo{
          .srcLayout = vk::ImageLayout::eTransferDstOptimal,
          .srcAccess = vk::AccessFlagBits2::eTransferWrite,
          .srcStage = vk::PipelineStageFlagBits2::eTransfer,
          .dstLayout = barrierInfo.dstLayout,
          .dstAccess = barrierInfo.dstAccess,
          .dstStage = barrierInfo.dstStage,
      };
      generateMipmaps(cb, *this, mip_barrier_info);
    };

    execute_command_and_wait(device, commandPool, queue, k_copy_and_mip);
  }
};
};  // namespace vku
