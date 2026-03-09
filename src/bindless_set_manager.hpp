#pragma once

#include "vku/texture/texture.hpp"
#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/gpu.hpp"

namespace lvk {
class BindlessSetManager {
  using Gpu = vkit::vulkan::Gpu;
  using Texture = vku::Texture;
  using BindlessLayout = vkit::vulkan::dsl::BindlessLayout;

 public:
  constexpr static auto kLinearSamplerId = 0;
  constexpr static auto kNearestSamplerId = 1;

  explicit BindlessSetManager(const Gpu& gpu, const BindlessLayout& bindless
                              [[clang::lifetimebound]])
      : bindless_{bindless},
        pool_{createDescriptorPool(*gpu.device)},
        set_{createDescriptorSet(*gpu.device)} {
    initDefaultSamplers(*gpu.device,
                        gpu.properties.limits.maxSamplerAnisotropy);
  }

  auto getSet() const -> vk::DescriptorSet { return *set_; }

  void addTexture2D(vk::Device device, const std::uint32_t id,
                    const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kTexture2DBindingIdx)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

  void addTextureCube(vk::Device device, const std::uint32_t id,
                      const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kTextureCubeBindingIdx)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

  void addSampler(vk::Device device, const std::uint32_t id,
                  vk::Sampler sampler) {
    auto image_info = vk::DescriptorImageInfo{};
    image_info.setSampler(sampler);

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kSamplerBindingIdx)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

 private:
  const BindlessLayout& bindless_;

  vk::UniqueDescriptorPool pool_;
  vk::UniqueDescriptorSet set_;
  std::vector<vk::UniqueSampler> samplers_;

  static auto createDescriptorPool(vk::Device device)
      -> vk::UniqueDescriptorPool {
    const auto pool_sizes_bindless = std::array<vk::DescriptorPoolSize, 2>{
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                               2 * BindlessLayout::kMaxTextures},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler,
                               BindlessLayout::kMaxSamplers},
    };

    auto pool_info = vk::DescriptorPoolCreateInfo{};
    pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
        .setMaxSets(1)
        .setPoolSizes(pool_sizes_bindless);

    return device.createDescriptorPoolUnique(pool_info);
  }

  auto createDescriptorSet(vk::Device device) -> vk::UniqueDescriptorSet {
    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*pool_).setSetLayouts(*bindless_);

    auto descriptor_counts =
        std::array<const std::uint32_t, 1>{BindlessLayout::kMaxTextures};

    auto count_info = vk::DescriptorSetVariableDescriptorCountAllocateInfo{};
    count_info.setDescriptorCounts(descriptor_counts);

    return std::move(device.allocateDescriptorSetsUnique(
        vk::StructureChain{alloc_info, count_info}.get())[0]);
  }

  void initDefaultSamplers(vk::Device device, float maxAnisotropy) {
    {
      auto sampler_ci = vk::SamplerCreateInfo{};
      sampler_ci.setMagFilter(vk::Filter::eLinear)
          .setMinFilter(vk::Filter::eLinear)
          .setMipmapMode(vk::SamplerMipmapMode::eLinear)
          .setAnisotropyEnable(vk::True)
          .setMaxAnisotropy(maxAnisotropy);

      auto sampler = device.createSamplerUnique(sampler_ci);
      addSampler(device, kLinearSamplerId, *sampler);
      samplers_.emplace_back(std::move(sampler));
    }

    {
      auto sampler_ci = vk::SamplerCreateInfo{};
      sampler_ci.setMagFilter(vk::Filter::eNearest)
          .setMinFilter(vk::Filter::eNearest)
          .setMipmapMode(vk::SamplerMipmapMode::eNearest)
          .setAnisotropyEnable(vk::True)
          .setMaxAnisotropy(maxAnisotropy);

      auto sampler = device.createSamplerUnique(sampler_ci);
      addSampler(device, kNearestSamplerId, *sampler);
      samplers_.emplace_back(std::move(sampler));
    }
  }
};
};  // namespace lvk
