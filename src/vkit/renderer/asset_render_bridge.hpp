#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/graphics/descriptor_buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"

namespace vkit::renderer {

class AssetRenderBridge {
 public:
  static constexpr std::size_t kMaxPrimitives = 10000;
  static constexpr std::size_t kMaxJoints = 10000;

  AssetRenderBridge(graphics::GfxDevice& device,
                    const dsl::PrimitiveSetLayout& layout,
                    std::uint32_t framesInFlight);

  void update(std::uint32_t frameIndex, asset::Asset* currentAsset);

  [[nodiscard]] auto getDescriptorSet(std::uint32_t frameIndex) const
      -> vk::DescriptorSet {
    return frames_[frameIndex].descriptorSet;
  }

 private:
  struct Frame {
    std::unique_ptr<graphics::DescriptorBuffer> primitiveSSBO;
    std::unique_ptr<graphics::DescriptorBuffer> jointSSBO;
    vk::DescriptorSet descriptorSet;
    std::optional<std::uint32_t> lastRenderedAssetId{std::nullopt};
  };

  graphics::GfxDevice& device_;
  std::vector<Frame> frames_;
  vk::UniqueDescriptorPool pool_;
};

};  // namespace vkit::renderer
