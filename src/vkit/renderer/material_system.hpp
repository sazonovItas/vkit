#pragma once

#include <memory>
#include <vector>

#include "vkit/graphics/descriptor_buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"

namespace vkit::renderer {

class MaterialSystem {
 public:
  static constexpr std::size_t kMaxMaterials = 1000;

  MaterialSystem(graphics::GfxDevice& device,
                 const dsl::MaterialSetLayout& layout,
                 std::uint32_t framesInFlight);

  void update(std::uint32_t frameIndex,
              const material::MaterialManager& manager);

  [[nodiscard]] auto getDescriptorSet(std::uint32_t frameIndex) const
      -> vk::DescriptorSet {
    return frames_[frameIndex].descriptorSet;
  }

 private:
  struct Frame {
    std::unique_ptr<graphics::DescriptorBuffer> diffuseBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> diffuseSpecBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> principledBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> mixBuffer;
    vk::DescriptorSet descriptorSet;
  };

  graphics::GfxDevice& device_;
  std::vector<Frame> frames_;
  vk::UniqueDescriptorPool pool_;
};

};  // namespace vkit::renderer
