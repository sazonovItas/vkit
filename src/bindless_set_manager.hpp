#pragma once

#include "texture.hpp"
#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/gpu.hpp"

namespace lvk {
class BindlessSetManager {
  using Bindless = vkit::vulkan::dsl::Bindless;

 public:
  constexpr static auto kLinearSamplerId = 0;
  constexpr static auto kNearestSamplerId = 1;

  vkit::vulkan::dsl::Bindless bindless;

  vk::UniqueDescriptorPool pool;
  vk::UniqueDescriptorSet set;
  std::vector<vk::UniqueSampler> samplers;

  explicit BindlessSetManager(const vkit::vulkan::Gpu& gpu)
      : bindless{*gpu.device},
        pool{createDescriptorPool(*gpu.device)},
        set{createDescriptorSet(*gpu.device)} {
    initDefaultSamplers(*gpu.device,
                        gpu.properties.limits.maxSamplerAnisotropy);
  }

  void addTexture2D(vk::Device device, const std::uint32_t id,
                    const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto writes = std::array<vk::WriteDescriptorSet, 1>{};
    writes[0]
        .setDstSet(*set)
        .setDstBinding(Bindless::kTexture2DBinding)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(writes, nullptr);
  }

  void addTextureCube(vk::Device device, const std::uint32_t id,
                      const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto writes = std::array<vk::WriteDescriptorSet, 1>{};
    writes[0]
        .setDstSet(*set)
        .setDstBinding(Bindless::kTextureCubeBinding)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(writes, nullptr);
  }

  void addSampler(vk::Device device, const std::uint32_t id,
                  vk::Sampler sampler) {
    auto image_info = vk::DescriptorImageInfo{};
    image_info.setSampler(sampler).setImageLayout(
        vk::ImageLayout::eShaderReadOnlyOptimal);

    auto writes = std::array<vk::WriteDescriptorSet, 1>{};
    writes[0]
        .setDstSet(*set)
        .setDstBinding(Bindless::kSamplerBinding)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setImageInfo(image_info);

    device.updateDescriptorSets(writes, nullptr);
  }

 private:
  static auto createDescriptorPool(vk::Device device)
      -> vk::UniqueDescriptorPool {
    const auto pool_sizes_bindless = std::array<vk::DescriptorPoolSize, 2>{
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                               2 * Bindless::kMaxBindlessResources},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler,
                               Bindless::kMaxSamplers},
    };

    auto pool_info = vk::DescriptorPoolCreateInfo{};
    pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
        .setMaxSets(1)
        .setPoolSizes(pool_sizes_bindless);

    return device.createDescriptorPoolUnique(pool_info);
  }

  auto createDescriptorSet(vk::Device device) -> vk::UniqueDescriptorSet {
    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*pool).setDescriptorSetCount(1).setSetLayouts(
        *bindless);

    auto descriptor_counts =
        std::array<const std::uint32_t, 1>{Bindless::kMaxBindlessResources};

    auto count_info = vk::DescriptorSetVariableDescriptorCountAllocateInfo{};
    count_info.setDescriptorSetCount(1).setDescriptorCounts(descriptor_counts);

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
      samplers.push_back(std::move(sampler));
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
      samplers.push_back(std::move(sampler));
    }
  }
};
};  // namespace lvk
