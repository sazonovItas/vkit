#pragma once

#include <vulkan/vulkan.hpp>

#include "vkit/graphics/command.hpp"
#include "vkit/renderer/pipeline_layout/light_gizmo.hpp"

namespace vkit::renderer::cmd {

class DrawLightGizmosCommand final : public graphics::Command {
 public:
  DrawLightGizmosCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                         vk::DescriptorSet sceneSet, std::uint32_t lightCount,
                         float size = 0.025F)
      : pipeline_{pipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        lightCount_{lightCount},
        size_{size} {}

  void record(vk::CommandBuffer cb) const override {
    if (lightCount_ == 0) return;
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0,
                          sceneSet_, nullptr);
    auto pcs = pl::LightGizmoPipelineLayout::PushConstants{.size = size_};
    cb.pushConstants(layout_, vk::ShaderStageFlagBits::eVertex, 0,
                     sizeof(pcs), &pcs);
    // 6 vertices per gizmo quad, lightCount_ instances
    cb.draw(6, lightCount_, 0, 0);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  std::uint32_t lightCount_;
  float size_;
};

};  // namespace vkit::renderer::cmd
