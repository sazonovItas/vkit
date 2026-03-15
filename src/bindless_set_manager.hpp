#pragma once

#include "vku/texture/texture.hpp"
#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit {
class BindlessSetManager {
  using Gpu = vkit::vulkan::Gpu;
  using Texture = vku::Texture;
  using BindlessLayout = vkit::vulkan::dsl::BindlessLayout;

 public:
  constexpr static auto kLinearSamplerId = 0;
  constexpr static auto kNearestSamplerId = 1;

  constexpr static auto kEnvMapTextureIdOffset = 768;

  explicit BindlessSetManager(const Gpu& gpu, const BindlessLayout& bindless
                              [[clang::lifetimebound]])
      : bindless_{bindless},
        pool_{createDescriptorPool(*gpu.device)},
        set_{createDescriptorSet(*gpu.device)} {
    initDefaultSamplers(*gpu.device,
                        gpu.properties.limits.maxSamplerAnisotropy);
  }

  auto getSet() const -> vk::DescriptorSet { return *set_; }

  static auto getEnvMapId(const std::int32_t id) -> std::int32_t {
    return kEnvMapTextureIdOffset + (3 * id);
  }

  static auto getDiffuseEnvMapId(const std::int32_t id) -> std::int32_t {
    return kEnvMapTextureIdOffset + (3 * id) + 1;
  }

  static auto getSpecularEnvMapId(const std::int32_t id) -> std::int32_t {
    return kEnvMapTextureIdOffset + (3 * id) + 2;
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

  void addStorageImage2D(vk::Device device, const std::uint32_t id,
                         const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kStorageImageBindingIdx)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

  void addEnvMapTexture2D(vk::Device device, const std::uint32_t id,
                          const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kTexture2DBindingIdx)
        .setDstArrayElement(kEnvMapTextureIdOffset + (3 * id))
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

  void addDiffuseEnvMapTexture2D(vk::Device device, const std::uint32_t id,
                                 const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kTexture2DBindingIdx)
        .setDstArrayElement(kEnvMapTextureIdOffset + (3 * id) + 1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setImageInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

  void addSpecularEnvMapTexture2D(vk::Device device, const std::uint32_t id,
                                  const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(BindlessLayout::kTexture2DBindingIdx)
        .setDstArrayElement(kEnvMapTextureIdOffset + (3 * id) + 2)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
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
    const auto pool_sizes_bindless = std::array<vk::DescriptorPoolSize, 3>{
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                               BindlessLayout::kMaxTextures},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler,
                               BindlessLayout::kMaxSamplers},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage,
                               BindlessLayout::kMaxStorageImages},
    };

    auto pool_info = vk::DescriptorPoolCreateInfo{};
    pool_info
        .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                  vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(1)
        .setPoolSizes(pool_sizes_bindless);

    return device.createDescriptorPoolUnique(pool_info);
  }

  auto createDescriptorSet(vk::Device device) -> vk::UniqueDescriptorSet {
    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*pool_).setSetLayouts(*bindless_);

    return std::move(device.allocateDescriptorSetsUnique(alloc_info)[0]);
  }

  void initDefaultSamplers(vk::Device device, float maxAnisotropy) {
    {
      auto sampler_ci = vk::SamplerCreateInfo{};
      sampler_ci.setMagFilter(vk::Filter::eLinear)
          .setMinFilter(vk::Filter::eLinear)
          .setMipmapMode(vk::SamplerMipmapMode::eLinear)
          .setAnisotropyEnable(vk::True)
          .setMaxAnisotropy(maxAnisotropy)
          .setMinLod(0.0F)
          .setMaxLod(vk::LodClampNone);

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
          .setMaxAnisotropy(maxAnisotropy)
          .setMinLod(0.0F)
          .setMaxLod(vk::LodClampNone);

      auto sampler = device.createSamplerUnique(sampler_ci);
      addSampler(device, kNearestSamplerId, *sampler);
      samplers_.emplace_back(std::move(sampler));
    }
  }
};
};  // namespace vkit
