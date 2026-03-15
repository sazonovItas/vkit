#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "stb_image.h"
#include "vk_mem_alloc.hpp"
#include "vku/commands.hpp"
#include "vku/images/allocated_image.hpp"
#include "vku/utils/utils.hpp"

namespace vku {
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

struct TextureInfo {
  int width;
  int height;
  vk::Format format{vk::Format::eR8G8B8A8Srgb};
  int channels{4};
  int bytePerChannel{1};
  std::vector<std::byte> bytes;

  static auto fromPath(const std::filesystem::path& path,
                       vk::Format format = vk::Format::eR8G8B8A8Srgb,
                       int channels = 4, int bytePerChannel = 1)
      -> TextureInfo {
    auto buffer = loadFileFromBinary(path);
    return fromBuffer(buffer.data(), buffer.size(), format, channels,
                      bytePerChannel);
  }

  static auto fromBuffer(const std::byte* buffer, const std::size_t size,
                         const vk::Format format = vk::Format::eR8G8B8A8Srgb,
                         const int channels = 4, const int bytePerChannel = 1)
      -> TextureInfo {
    int width;
    int height;
    int original_channels;

    void* pixels;
    switch (bytePerChannel) {
      case 1:
        pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer),
                                       size, &width, &height,
                                       &original_channels, channels);
        break;

      case 2:
        pixels = stbi_load_16_from_memory(
            reinterpret_cast<const stbi_uc*>(buffer), size, &width, &height,
            &original_channels, channels);
        break;

      case 4:
        pixels = stbi_loadf_from_memory(
            reinterpret_cast<const stbi_uc*>(buffer), size, &width, &height,
            &original_channels, channels);
        break;

      default:
        throw std::runtime_error{std::format(
            "wrong amount of byte per channle: {}", bytePerChannel)};
    }
    if (pixels == nullptr) {
      throw std::runtime_error{
          std::format("failed to load image: {}", stbi_failure_reason())};
    }

    size_t byte_count = width * height * channels * bytePerChannel;

    auto info = TextureInfo{
        .width = width,
        .height = height,
        .format = format,
        .channels = channels,
        .bytePerChannel = bytePerChannel,
        .bytes = std::vector<std::byte>(
            static_cast<std::byte*>(pixels),
            static_cast<std::byte*>(pixels) + byte_count),
    };

    stbi_image_free(pixels);

    return info;
  }

  static auto fromRawBuffer(const std::byte* buffer, const int width,
                            const int height,
                            const vk::Format format = vk::Format::eR8G8B8A8Srgb,
                            const int channels = 4,
                            const int bytePerChannel = 1) -> TextureInfo {
    size_t byte_count = width * height * channels * bytePerChannel;

    auto info = TextureInfo{
        .width = width,
        .height = height,
        .format = format,
        .channels = channels,
        .bytePerChannel = bytePerChannel,
        .bytes = std::vector<std::byte>(buffer, buffer + byte_count),
    };

    return info;
  }

 private:
  static auto loadFileFromBinary(const std::filesystem::path& path)
      -> std::vector<std::byte> {
    auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
    if (!file) {
      throw std::runtime_error{"failed to open file: " + path.string()};
    }

    const auto size = file.tellg();
    if (size < 0) {
      throw std::runtime_error{"failed to determine file size: " +
                               path.string()};
    }

    std::vector<std::byte> buffer(static_cast<std::size_t>(size));

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
      throw std::runtime_error{"failed to read file: " + path.string()};
    }

    return buffer;
  }
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
      vma::Allocator allocator, const vku::DeviceCopyInfo& copyInfo,
      const int width, const int height,
      const vk::Format format = vk::Format::eR8G8B8A8Srgb,
      const std::uint32_t mipLevels = 1,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2 accessFlags = vk::AccessFlagBits2::eShaderRead,
      vk::Flags<vk::PipelineStageFlagBits2> stageFlags =
          vk::PipelineStageFlagBits2::eFragmentShader) {
    auto image_ci = vk::ImageCreateInfo{};
    image_ci.setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setMipLevels(mipLevels)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setExtent(vk::Extent3D{
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height),
            1,
        })
        .setUsage(imageUsageFlags | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst);

    image = vku::AllocatedImage{allocator, image_ci};

    auto transfer_image_to_target_layout_fn = [&](vk::CommandBuffer cb) {
      auto subresource_range = vk::ImageSubresourceRange{};
      subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
          .setLevelCount(mipLevels)
          .setLayerCount(1);

      {
        auto barrier = vk::ImageMemoryBarrier2{};
        barrier.setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(imageLayout)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstAccessMask(accessFlags)
            .setDstStageMask(stageFlags)
            .setImage(image.image)
            .setSubresourceRange(subresource_range);

        cb.pipelineBarrier2(
            vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
      }
    };

    vku::executeCommandAndWait(copyInfo.device, copyInfo.commandPool,
                               copyInfo.queue,
                               transfer_image_to_target_layout_fn);

    this->imageLayout = imageLayout;
    imageView =
        copyInfo.device.createImageViewUnique(image.getViewCreateInfo());
  }

  Texture2D(
      const TextureInfo& info, vma::Allocator allocator,
      const vku::DeviceCopyInfo& copyInfo, const std::uint32_t mipLevels = 1,
      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
      vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2 accessFlags = vk::AccessFlagBits2::eShaderRead,
      vk::Flags<vk::PipelineStageFlagBits2> stageFlags =
          vk::PipelineStageFlagBits2::eFragmentShader) {
    populate(info, allocator, copyInfo, mipLevels, imageUsageFlags, imageLayout,
             accessFlags, stageFlags);
  }

 private:
  void populate(const TextureInfo& info, vma::Allocator allocator,
                const vku::DeviceCopyInfo& copyInfo, std::uint32_t mipLevels,
                vk::ImageUsageFlags imageUsageFlags,
                vk::ImageLayout imageLayout, vk::AccessFlagBits2 accessFlags,
                vk::Flags<vk::PipelineStageFlagBits2> stageFlags);
};
};  // namespace vku
