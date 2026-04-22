#include "vkit/graphics/texture.hpp"

#include <stdexcept>

#include "vkit/graphics/enums.hpp"
#include "vkit/graphics/image.hpp"
#include "vkit/graphics/util.hpp"

namespace {

auto getExtent3D(int width, int height, int depth) -> vk::Extent3D {
  return vk::Extent3D{
      static_cast<std::uint32_t>(width),
      static_cast<std::uint32_t>(height),
      static_cast<std::uint32_t>(depth),
  };
}

};  // namespace

namespace vkit::graphics {

auto getTextureLevelCount(int width, int height, int depth) -> int {
  return Image::maxMipLevels(getExtent3D(width, height, depth));
}

Texture::Texture(vma::Allocator allocator,
                 const util::RecordAndSubmitInfo& rsInfo,
                 const TextureCreateInfo& createInfo)
    : type_{createInfo.type},
      sampleCount_{createInfo.sampleCount},
      useMipmaps_{createInfo.useMipmaps},
      image_{createAllocatedImage(allocator, createInfo)},
      view_{makeImageView(rsInfo.device)} {
  if (createInfo.buffer != nullptr) {
    update(rsInfo, *createInfo.buffer);
  }

  if (useMipmaps_) {
    generateMipmaps(rsInfo);
  }
}

auto Texture::makeImageView(vk::Device device) const -> vk::UniqueImageView {
  auto ci = image_.getViewCreateInfo(toVkImageViewType(type_));
  return device.createImageViewUnique(ci);
}

auto Texture::makeImageView(vk::Device device, std::uint32_t baseMipLevel,
                            std::uint32_t levelCount,
                            std::uint32_t baseArrayLayer,
                            std::uint32_t layerCount) const
    -> vk::UniqueImageView {
  const auto aspect = Image::inferAspectFlags(image_.format);

  vk::ImageSubresourceRange subresource{};
  subresource.setAspectMask(aspect)
      .setBaseMipLevel(baseMipLevel)
      .setLevelCount(levelCount)
      .setBaseArrayLayer(baseArrayLayer)
      .setLayerCount(layerCount);

  auto ci = image_.getViewCreateInfo(subresource, toVkImageViewType(type_));

  return device.createImageViewUnique(ci);
}

auto Texture::getImage() const -> Image { return image_; };

auto Texture::getView() const -> vk::ImageView { return *view_; }

auto Texture::getTextureType() const -> TextureType { return type_; }

auto Texture::getPixelFormat() const -> dataformat::Format {
  return image_.format;
}

auto Texture::getWidth(std::uint32_t level) const -> int {
  return Image::mipExtent(image_.extent, level).width;
}

auto Texture::getHeight(std::uint32_t level) const -> int {
  return Image::mipExtent(image_.extent, level).height;
}

auto Texture::getDepth(std::uint32_t level) const -> int {
  return Image::mipExtent(image_.extent, level).depth;
}

auto Texture::getArrayLayerCount() const -> int { return image_.arrayLayers; }

auto Texture::getLevelCount() const -> int { return image_.mipLevels; }

auto Texture::getSampleCount() const -> SampleCount { return sampleCount_; }

auto Texture::isLayered() const -> bool { return image_.arrayLayers > 1; }

void Texture::update(const util::RecordAndSubmitInfo& info,
                     const Buffer& buffer) {
  auto image_size_bytes = image_.getBaseMipSizeBytes();
  if (buffer.size != image_size_bytes) {
    throw std::runtime_error{
        std::format("size in bytes of the buffer and image are mismatched: "
                    "buffer size {}, "
                    "image size {}",
                    buffer.size, image_size_bytes)};
  }

  auto fn = [&](vk::CommandBuffer cb) {
    {
      vk::ImageMemoryBarrier barrier{};
      barrier.setOldLayout(vk::ImageLayout::eUndefined)
          .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setImage(image_.image)
          .setSubresourceRange(image_.subresourceRange())
          .setSrcAccessMask({})
          .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

      cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                         vk::PipelineStageFlagBits::eTransfer, {}, nullptr,
                         nullptr, barrier);
    }

    vk::ImageSubresourceLayers image_subresource_layers{};
    image_subresource_layers
        .setAspectMask(Image::inferAspectFlags(image_.format))
        .setBaseArrayLayer(0)
        .setLayerCount(image_.arrayLayers)
        .setMipLevel(0);

    vk::BufferImageCopy2 region{};
    region.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(image_subresource_layers)
        .setImageOffset({0, 0, 0})
        .setImageExtent(image_.extent);

    vk::CopyBufferToImageInfo2 copy_info{};
    copy_info.setSrcBuffer(buffer.buffer)
        .setDstImage(image_.image)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(region);

    cb.copyBufferToImage2(copy_info);

    {
      vk::ImageMemoryBarrier barrier{};
      barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
          .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setImage(image_.image)
          .setSubresourceRange(image_.subresourceRange())
          .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
          .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

      cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                         vk::PipelineStageFlagBits::eFragmentShader, {},
                         nullptr, nullptr, barrier);
    }
  };

  util::recordAndSubmit(info, fn);
}

void Texture::generateMipmaps(const util::RecordAndSubmitInfo& info) {
  if (!useMipmaps_) {
    return;
  }

  auto fn = [&](vk::CommandBuffer cb) {
    const auto format = image_.format;

    const auto aspect = Image::inferAspectFlags(format);

    for (uint32_t i = 1; i < image_.mipLevels; ++i) {
      {
        vk::ImageMemoryBarrier barrier{};
        barrier.setImage(image_.image)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange{aspect, i - 1, 1, 0,
                                                           image_.arrayLayers})
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eTransfer, {}, nullptr,
                           nullptr, barrier);
      }

      auto src_extent = image_.mipExtent(i - 1);
      auto dst_extent = image_.mipExtent(i);

      vk::ImageBlit blit{};
      blit.setSrcSubresource(
          vk::ImageSubresourceLayers{aspect, i - 1, 0, image_.arrayLayers});
      blit.setDstSubresource(
          vk::ImageSubresourceLayers{aspect, i, 0, image_.arrayLayers});

      blit.setSrcOffsets({
          vk::Offset3D{0, 0, 0},
          vk::Offset3D{
              static_cast<int32_t>(src_extent.width),
              static_cast<int32_t>(src_extent.height),
              static_cast<int32_t>(src_extent.depth),
          },
      });

      blit.setDstOffsets({
          vk::Offset3D{0, 0, 0},
          vk::Offset3D{
              static_cast<int32_t>(dst_extent.width),
              static_cast<int32_t>(dst_extent.height),
              static_cast<int32_t>(dst_extent.depth),
          },
      });

      cb.blitImage(image_.image, vk::ImageLayout::eTransferSrcOptimal,
                   image_.image, vk::ImageLayout::eTransferDstOptimal, blit,
                   vk::Filter::eLinear);

      {
        vk::ImageMemoryBarrier barrier{};
        barrier.setImage(image_.image)
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange{aspect, i - 1, 1, 0,
                                                           image_.arrayLayers})
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eFragmentShader, {},
                           nullptr, nullptr, barrier);
      }
    }

    {
      vk::ImageMemoryBarrier barrier{};
      barrier.setImage(image_.image)
          .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
          .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setSubresourceRange(vk::ImageSubresourceRange{
              aspect, image_.mipLevels - 1, 1, 0, image_.arrayLayers})
          .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
          .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

      cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                         vk::PipelineStageFlagBits::eFragmentShader, {},
                         nullptr, nullptr, barrier);
    }
  };

  util::recordAndSubmit(info, fn);
}

auto Texture::createAllocatedImage(vma::Allocator allocator,
                                   const TextureCreateInfo& createInfo)
    -> AllocatedImage {
  auto mip_levels = createInfo.useMipmaps ? createInfo.levelCount : 1;

  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(toVkImageType(createInfo.type))
      .setFormat(createInfo.pixelFormat)
      .setExtent(
          getExtent3D(createInfo.width, createInfo.height, createInfo.depth))
      .setSamples(getSampleCountFlagBits(createInfo.sampleCount))
      .setMipLevels(mip_levels)
      .setArrayLayers(createInfo.arrayLayerCount);

  return AllocatedImage{allocator, image_ci};
}

};  // namespace vkit::graphics
