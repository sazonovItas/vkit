#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "vkit/graphics/texture.hpp"

namespace vkit::graphics {

class BindlessTextureManager {
 public:
  static constexpr std::uint32_t kSamplerBinding = 0;
  static constexpr std::uint32_t kTextureBinding = 1;

  static constexpr std::uint32_t kLinearSamplerId = 0;
  static constexpr std::uint32_t kNearestSamplerId = 1;

  BindlessTextureManager(vk::Device device, vk::DescriptorSet bindlessSet,
                         std::uint32_t maxSamplers, std::uint32_t maxTextures,
                         std::shared_ptr<Texture> dummyTexture)
      : device_(device),
        bindlessSet_(bindlessSet),
        maxSamplers_(maxSamplers),
        maxTextures_(maxTextures),
        dummyTexture_(std::move(dummyTexture)) {
    if (!dummyTexture_) {
      throw std::invalid_argument(
          "Dummy texture must be provided to BindlessTextureManager.");
    }

    textures_.resize(maxTextures_);
    freeTextureIndices_.reserve(maxTextures_);

    for (std::uint32_t i = 0; i < maxTextures_; ++i) {
      freeTextureIndices_.push_back(maxTextures_ - 1 - i);
      updateTextureDescriptor(i, dummyTexture_);
    }

    samplers_.resize(maxSamplers_);
    freeSamplerIndices_.reserve(maxSamplers_);
    for (std::uint32_t i = maxSamplers_; i > 2; --i) {
      freeSamplerIndices_.push_back(i - 1);
    }

    createDefaultSamplers();
  }

  [[nodiscard]] auto addTexture(const std::shared_ptr<Texture>& texture)
      -> std::uint32_t {
    if (!texture) {
      throw std::invalid_argument(
          "Cannot add null texture to bindless manager");
    }
    if (freeTextureIndices_.empty()) {
      throw std::runtime_error("Bindless texture array capacity reached!");
    }

    std::uint32_t index = freeTextureIndices_.back();
    freeTextureIndices_.pop_back();

    textures_[index] = texture;
    updateTextureDescriptor(index, texture);

    return index;
  }

  void removeTexture(std::uint32_t index) {
    if (index >= maxTextures_ || !textures_[index]) return;

    updateTextureDescriptor(index, dummyTexture_);

    textures_[index].reset();
    freeTextureIndices_.push_back(index);
  }

  [[nodiscard]] auto addSampler(const vk::SamplerCreateInfo& samplerInfo)
      -> std::uint32_t {
    if (freeSamplerIndices_.empty()) {
      throw std::runtime_error("Bindless sampler array capacity reached!");
    }

    std::uint32_t index = freeSamplerIndices_.back();
    freeSamplerIndices_.pop_back();

    samplers_[index] = device_.createSamplerUnique(samplerInfo);
    updateSamplerDescriptor(index, samplers_[index].get());

    return index;
  }

  void removeSampler(std::uint32_t index) {
    if (index < 2 || index >= maxSamplers_ || !samplers_[index]) return;

    samplers_[index].reset();
    freeSamplerIndices_.push_back(index);
  }

 private:
  vk::Device device_;
  vk::DescriptorSet bindlessSet_;

  std::uint32_t maxSamplers_;
  std::uint32_t maxTextures_;

  std::shared_ptr<Texture> dummyTexture_;

  std::vector<std::shared_ptr<Texture>> textures_;
  std::vector<std::uint32_t> freeTextureIndices_;

  std::vector<vk::UniqueSampler> samplers_;
  std::vector<std::uint32_t> freeSamplerIndices_;

  void createDefaultSamplers() {
    auto linear_info = vk::SamplerCreateInfo{}
                           .setMagFilter(vk::Filter::eLinear)
                           .setMinFilter(vk::Filter::eLinear)
                           .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                           .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                           .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                           .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                           .setAnisotropyEnable(vk::True)
                           .setMaxAnisotropy(16.0F);

    samplers_[kLinearSamplerId] = device_.createSamplerUnique(linear_info);
    updateSamplerDescriptor(kLinearSamplerId,
                            samplers_[kLinearSamplerId].get());

    auto nearest_info = vk::SamplerCreateInfo{}
                            .setMagFilter(vk::Filter::eNearest)
                            .setMinFilter(vk::Filter::eNearest)
                            .setMipmapMode(vk::SamplerMipmapMode::eNearest)
                            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                            .setAnisotropyEnable(vk::False);

    samplers_[kNearestSamplerId] = device_.createSamplerUnique(nearest_info);
    updateSamplerDescriptor(kNearestSamplerId,
                            samplers_[kNearestSamplerId].get());
  }

  void updateTextureDescriptor(std::uint32_t index,
                               const std::shared_ptr<Texture>& texture) {
    auto image_info =
        vk::DescriptorImageInfo{}
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(texture->getImageView());

    auto write = vk::WriteDescriptorSet{}
                     .setDstSet(bindlessSet_)
                     .setDstBinding(kTextureBinding)
                     .setDstArrayElement(index)
                     .setDescriptorType(vk::DescriptorType::eSampledImage)
                     .setDescriptorCount(1)
                     .setPImageInfo(&image_info);

    device_.updateDescriptorSets(write, {});
  }

  void updateSamplerDescriptor(std::uint32_t index, vk::Sampler sampler) {
    auto sampler_info = vk::DescriptorImageInfo{}.setSampler(sampler);

    auto write = vk::WriteDescriptorSet{}
                     .setDstSet(bindlessSet_)
                     .setDstBinding(kSamplerBinding)
                     .setDstArrayElement(index)
                     .setDescriptorType(vk::DescriptorType::eSampler)
                     .setDescriptorCount(1)
                     .setPImageInfo(&sampler_info);

    device_.updateDescriptorSets(write, {});
  }
};

};  // namespace vkit::graphics
