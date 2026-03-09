#pragma once

#include "vku/texture/texture.hpp"
#include "vulkan/descriptor_set_layout/material.hpp"
#include "vulkan/gpu.hpp"

namespace lvk {
class BindlessSetManager {
  using Gpu = vkit::vulkan::Gpu;
  using Texture = vku::Texture;
  using MaterialLayout = vkit::vulkan::dsl::MaterialLayout;

 public:
  constexpr static auto kLinearSamplerId = 0;
  constexpr static auto kNearestSamplerId = 1;

  explicit BindlessSetManager(const Gpu& gpu,
                              const MaterialLayout& materialLayout
                              [[clang::lifetimebound]])
      : material_{materialLayout},
        pool_{createDescriptorPool(*gpu.device)},
        set_{createDescriptorSet(*gpu.device)} {}

  auto getSet() -> vk::DescriptorSet { return *set_; }

  void addTexture2D(vk::Device device, const std::uint32_t id,
                    const Texture& texture) {
    auto image_info = texture.descriptorInfo();

    auto write = vk::WriteDescriptorSet{};
    write.setDstSet(*set_)
        .setDstBinding(MaterialLayout::kMaterialBindingIdx)
        .setDstArrayElement(id)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
        .setBufferInfo(image_info);

    device.updateDescriptorSets(write, nullptr);
  }

 private:
  const MaterialLayout& material_;

  vk::UniqueDescriptorPool pool_;
  vk::UniqueDescriptorSet set_;

  static auto createDescriptorPool(vk::Device device)
      -> vk::UniqueDescriptorPool {
    const auto pool_sizes_material = std::array<vk::DescriptorPoolSize, 2>{
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                               MaterialLayout::kMaxMaterials},
    };

    auto pool_info = vk::DescriptorPoolCreateInfo{};
    pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
        .setMaxSets(1)
        .setPoolSizes(pool_sizes_material);

    return device.createDescriptorPoolUnique(pool_info);
  }

  auto createDescriptorSet(vk::Device device) -> vk::UniqueDescriptorSet {
    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*pool_).setDescriptorSetCount(1).setSetLayouts(
        *material_);

    auto descriptor_counts =
        std::array<const std::uint32_t, 1>{MaterialLayout::kMaxMaterials};

    auto count_info = vk::DescriptorSetVariableDescriptorCountAllocateInfo{};
    count_info.setDescriptorSetCount(1).setDescriptorCounts(descriptor_counts);

    return std::move(device.allocateDescriptorSetsUnique(
        vk::StructureChain{alloc_info, count_info}.get())[0]);
  }
};
};  // namespace lvk
