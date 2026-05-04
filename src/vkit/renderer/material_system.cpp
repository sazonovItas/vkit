#include "vkit/renderer/material_system.hpp"

namespace vkit::renderer {

MaterialSystem::MaterialSystem(graphics::GfxDevice& device,
                               const dsl::MaterialSetLayout& layout,
                               std::uint32_t framesInFlight)
    : device_{device} {
  std::array<vk::DescriptorPoolSize, 1> pool_sizes = {vk::DescriptorPoolSize{
      vk::DescriptorType::eStorageBuffer,
      framesInFlight * 3,
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

    frame.diffuseBuffer = std::make_unique<graphics::DescriptorBuffer>(
        device_.allocator, usage,
        sizeof(material::Diffuse::Data) * kMaxMaterials);
    frame.diffuseSpecBuffer = std::make_unique<graphics::DescriptorBuffer>(
        device_.allocator, usage,
        sizeof(material::DiffuseSpecular::Data) * kMaxMaterials);
    frame.principledBuffer = std::make_unique<graphics::DescriptorBuffer>(
        device_.allocator, usage,
        sizeof(material::PrincipledBSDF::Data) * kMaxMaterials);

    vk::DescriptorSetAllocateInfo alloc_info{*pool_, 1, &layout.get()};
    frame.descriptorSet =
        device_.get().allocateDescriptorSets(alloc_info).front();

    auto diff_info = frame.diffuseBuffer->descriptorInfo();
    auto spec_info = frame.diffuseSpecBuffer->descriptorInfo();
    auto bsdf_info = frame.principledBuffer->descriptorInfo();

    std::array<vk::WriteDescriptorSet, 3> writes = {
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::MaterialSetLayout::kDiffuseBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &diff_info,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::MaterialSetLayout::kDiffuseSpecularBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &spec_info,
            nullptr,
        },
        vk::WriteDescriptorSet{
            frame.descriptorSet,
            dsl::MaterialSetLayout::kPrincipledBsdfBinding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &bsdf_info,
            nullptr,
        },
    };
    device_.get().updateDescriptorSets(writes, nullptr);
  }
}

void MaterialSystem::update(std::uint32_t frameIndex,
                            const material::MaterialManager& manager) {
  auto& frame = frames_[frameIndex];

  auto diffuse_data = manager.diffuse.getData();
  if (!diffuse_data.empty())
    std::ignore = frame.diffuseBuffer->writeAt(std::as_bytes(diffuse_data));

  auto spec_data = manager.diffuseSpecular.getData();
  if (!spec_data.empty())
    std::ignore = frame.diffuseSpecBuffer->writeAt(std::as_bytes(spec_data));

  auto bsdf_data = manager.principledBSDF.getData();
  if (!bsdf_data.empty())
    std::ignore = frame.principledBuffer->writeAt(std::as_bytes(bsdf_data));
}

};  // namespace vkit::renderer
