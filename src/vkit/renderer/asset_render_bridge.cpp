#include "vkit/renderer/asset_render_bridge.hpp"

namespace vkit::renderer {

AssetRenderBridge::AssetRenderBridge(graphics::GfxDevice& device,
                                     const dsl::PrimitiveSetLayout& layout,
                                     std::uint32_t framesInFlight)
    : device_{device} {
  std::array<vk::DescriptorPoolSize, 1> pool_sizes = {vk::DescriptorPoolSize{
      vk::DescriptorType::eStorageBuffer,
      framesInFlight * 2,
  }};

  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(framesInFlight)
      .setPoolSizes(pool_sizes);

  pool_ = device_.get().createDescriptorPoolUnique(pool_info);

  frames_.resize(framesInFlight);
  for (std::uint32_t i = 0; i < framesInFlight; ++i) {
    auto& frame = frames_[i];
    auto usage = static_cast<dataformat::BufferUsageFlags>(
        vk::BufferUsageFlagBits::eStorageBuffer);

    frame.primitiveSSBO = std::make_unique<graphics::DescriptorBuffer>(
        device_.allocator, usage,
        sizeof(primitive::PrimitiveData) * kMaxPrimitives);
    frame.jointSSBO = std::make_unique<graphics::DescriptorBuffer>(
        device_.allocator, usage, sizeof(glm::mat4) * kMaxJoints);

    vk::DescriptorSetAllocateInfo alloc_info{*pool_, 1, &layout.get()};
    frame.descriptorSet =
        device_.get().allocateDescriptorSets(alloc_info).front();

    auto prim_info = frame.primitiveSSBO->descriptorInfo();
    auto joint_info = frame.jointSSBO->descriptorInfo();

    std::array<vk::WriteDescriptorSet, 2> writes = {
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::PrimitiveSetLayout::kPrimitiveBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &prim_info,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::PrimitiveSetLayout::kJointBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &joint_info,
            nullptr,
        },
    };
    device_.get().updateDescriptorSets(writes, nullptr);
  }
}

void AssetRenderBridge::update(std::uint32_t frameIndex,
                               asset::Asset* currentAsset) {
  if (!currentAsset) return;

  auto& frame = frames_[frameIndex];

  auto current_asset_id = currentAsset->getStorageId();
  bool descriptors_need_update = false;

  if (frame.lastRenderedAssetId != current_asset_id) {
    descriptors_need_update = true;
    frame.lastRenderedAssetId = current_asset_id;
  }

  auto prim_data = currentAsset->primitives.getData();
  if (!prim_data.empty()) {
    if (frame.primitiveSSBO->writeAt(std::as_bytes(std::span{prim_data})))
      descriptors_need_update = true;
  }

  auto joint_data = currentAsset->skins.getData();
  if (!joint_data.empty()) {
    if (frame.jointSSBO->writeAt(std::as_bytes(std::span{joint_data})))
      descriptors_need_update = true;
  }

  if (descriptors_need_update && frame.descriptorSet) {
    auto prim_info = frame.primitiveSSBO->descriptorInfo();
    auto joint_info = frame.jointSSBO->descriptorInfo();

    std::array<vk::WriteDescriptorSet, 2> writes = {
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::PrimitiveSetLayout::kPrimitiveBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &prim_info,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::PrimitiveSetLayout::kJointBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &joint_info,
            nullptr,
        },
    };
    device_.get().updateDescriptorSets(writes, nullptr);
  }
}

};  // namespace vkit::renderer
