#include "image.hpp"

#include <print>

#include "buffer.hpp"
#include "vulkan/vulkan.hpp"

namespace {
void generate_mipmaps(vk::CommandBuffer cb, std::uint32_t queue_family,
                      vk::Image image, vk::Extent2D extent,
                      std::uint32_t mip_levels) {
  auto subresource_range = vk::ImageSubresourceRange{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setLevelCount(1);

  auto barrier = vk::ImageMemoryBarrier2{}
                     .setImage(image)
                     .setSrcQueueFamilyIndex(queue_family)
                     .setDstQueueFamilyIndex(queue_family)
                     .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                     .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                     .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                     .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
                                       vk::AccessFlagBits2::eMemoryWrite)
                     .setSubresourceRange(subresource_range);

  auto mip_width = extent.width;
  auto mip_height = extent.height;

  auto dependency_info = vk::DependencyInfo{};
  for (std::uint32_t i = 1; i < mip_levels; i++) {
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
    barrier.subresourceRange.setBaseMipLevel(i - 1);

    dependency_info.setImageMemoryBarriers(barrier);
    cb.pipelineBarrier2(dependency_info);

    auto src_offsets = std::array<vk::Offset3D, 2>{
        vk::Offset3D{0, 0, 0},
        vk::Offset3D{}.setX(mip_width).setY(mip_height).setZ(1),
    };
    auto src_subresource = vk::ImageSubresourceLayers{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(i - 1);

    auto dst_offsets = std::array<vk::Offset3D, 2>{
        vk::Offset3D{0, 0, 0},
        vk::Offset3D{}
            .setX(mip_width > 1 ? mip_width / 2 : 1)
            .setY(mip_height > 1 ? mip_height / 2 : 1)
            .setZ(1),
    };
    auto dst_subresource = vk::ImageSubresourceLayers{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(i);

    auto region = vk::ImageBlit2{}
                      .setSrcOffsets(src_offsets)
                      .setSrcSubresource(src_subresource)
                      .setDstOffsets(dst_offsets)
                      .setDstSubresource(dst_subresource);

    auto info = vk::BlitImageInfo2{}
                    .setSrcImage(image)
                    .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                    .setDstImage(image)
                    .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                    .setRegions(region)
                    .setFilter(vk::Filter::eLinear);

    cb.blitImage2(info);

    barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk ::AccessFlagBits2::eMemoryRead)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead);

    dependency_info.setImageMemoryBarriers(barrier);
    cb.pipelineBarrier2(dependency_info);

    mip_width = mip_width > 1 ? mip_width / 2 : 1;
    mip_height = mip_height > 1 ? mip_height / 2 : 1;
  }

  barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
  barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
      .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSrcAccessMask(vk ::AccessFlagBits2::eMemoryRead)
      .setDstAccessMask(vk::AccessFlagBits2::eShaderRead);

  dependency_info.setImageMemoryBarriers(barrier);
  cb.pipelineBarrier2(dependency_info);
}
};  // namespace

namespace vkit::vulkan::vma {
void ImageDeleter::operator()(RawImage const& raw_image) const noexcept {
  vmaDestroyImage(raw_image.allocator, raw_image.image, raw_image.allocation);
}

auto create_image(const ImageCreateInfo& create_info, vk::ImageUsageFlags usage,
                  std::uint32_t levels, vk::Format format, vk::Extent2D extent)
    -> Image {
  if (extent.width == 0 || extent.height == 0) {
    std::println(stderr, "images cannot have 0 width or height");
    return {};
  }

  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(vk::ImageType::e2D)
      .setExtent({extent.width, extent.height, 1})
      .setFormat(format)
      .setUsage(usage)
      .setArrayLayers(1)
      .setMipLevels(levels)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setTiling(vk::ImageTiling::eOptimal)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setQueueFamilyIndices(create_info.queue_family);
  auto const vk_image_ci = static_cast<VkImageCreateInfo>(image_ci);

  auto allocation_ci = VmaAllocationCreateInfo{};
  allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
  VkImage image{};
  VmaAllocation allocation{};
  auto const result = vmaCreateImage(create_info.allocator, &vk_image_ci,
                                     &allocation_ci, &image, &allocation, {});
  if (result != VK_SUCCESS) {
    std::println(stderr, "failed to create VMA Image");
    return {};
  }

  return RawImage{
      .allocator = create_info.allocator,
      .allocation = allocation,
      .image = image,
      .extent = extent,
      .format = format,
      .levels = levels,
  };
}

auto create_sampled_image(ImageCreateInfo const& create_info,
                          util::CommandBlock command_block,
                          Bitmap const& bitmap) -> Image {
  auto const mip_levels = create_info.levels;
  auto const usize = glm::uvec2{bitmap.size};
  auto const extent = vk::Extent2D{usize.x, usize.y};
  auto const usage =
      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  auto ret = create_image(create_info, usage, mip_levels,
                          vk::Format::eR8G8B8A8Srgb, extent);

  auto const buffer_ci = BufferCreateInfo{
      .allocator = create_info.allocator,
      .usage = vk::BufferUsageFlagBits::eTransferSrc,
      .queue_family = create_info.queue_family,
  };
  auto const staging_buffer = create_buffer(buffer_ci, BufferMemoryType::kHost,
                                            bitmap.bytes.size_bytes());

  if (!ret.get().image || !staging_buffer.get().buffer) {
    return {};
  }

  std::memcpy(staging_buffer.get().mapped, bitmap.bytes.data(),
              bitmap.bytes.size_bytes());

  auto dependency_info = vk::DependencyInfo{};
  auto subresource_range = vk::ImageSubresourceRange{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setLayerCount(1)
                               .setLevelCount(mip_levels);

  auto barrier = vk::ImageMemoryBarrier2{}
                     .setImage(ret.get().image)
                     .setSrcQueueFamilyIndex(create_info.queue_family)
                     .setDstQueueFamilyIndex(create_info.queue_family)
                     .setOldLayout(vk::ImageLayout::eUndefined)
                     .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                     .setSubresourceRange(subresource_range)
                     .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                     .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                     .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                     .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
                                       vk::AccessFlagBits2::eMemoryWrite);
  dependency_info.setImageMemoryBarriers(barrier);

  command_block.cb().pipelineBarrier2(dependency_info);

  auto subresource_layers = vk::ImageSubresourceLayers{}
                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setLayerCount(1)
                                .setMipLevel(0);
  auto buffer_image_copy =
      vk::BufferImageCopy2{}
          .setImageSubresource(subresource_layers)
          .setImageExtent(vk::Extent3D{extent.width, extent.height, 1});
  auto copy_info = vk::CopyBufferToImageInfo2{}
                       .setDstImage(ret.get().image)
                       .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setSrcBuffer(staging_buffer.get().buffer)
                       .setRegions(buffer_image_copy);

  command_block.cb().copyBufferToImage2(copy_info);

  generate_mipmaps(command_block.cb(), create_info.queue_family,
                   ret.get().image, ret.get().extent, mip_levels);

  command_block.submit_and_wait();

  return ret;
}
};  // namespace vkit::vulkan::vma
