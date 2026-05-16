#pragma once

#include <vulkan/vulkan.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/renderer/pipeline_layout/shadow.hpp"

namespace vkit::renderer::cmd {

class DrawShadowCommand final : public graphics::Command {
 public:
  DrawShadowCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                    vk::DescriptorSet lightSet, vk::DescriptorSet primSet,
                    const asset::Asset* asset,
                    const material::MaterialManager* materialManager,
                    bool enableSkinning)
      : pipeline_{pipeline},
        layout_{layout},
        lightSet_{lightSet},
        primSet_{primSet},
        asset_{asset},
        materialManager_{materialManager},
        enableSkinning_{enableSkinning} {}

  void record(vk::CommandBuffer cb) const override {
    if (!asset_ || asset_->scenes.empty()) return;
    auto active_scene = asset_->getActiveScene();
    if (!active_scene) return;

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

    std::array<vk::DescriptorSet, 2> sets = {lightSet_, primSet_};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    auto traverse_node = [&](auto& self, const scene::Node* node) -> void {
      if (node->mesh) {
        for (const auto& prim : node->mesh->getPrimitives()) {
          if (!prim->attrs.index.isValid() || !prim->getStorageId().has_value())
            continue;

          // Only cast shadows for opaque geometry
          if (materialManager_) {
            auto slot = materialManager_->getSlot(prim->getMaterialSlot());
            if (slot && slot->getMaterialType() != material::Type::kNone) {
              auto mat = materialManager_->getMaterial(slot->getMaterialType(),
                                                      slot->getMaterialId());
              if (mat && mat->getAlphaMode() == material::AlphaMode::kBlend)
                continue;
            }
          }

          const glm::mat4 model = node->getGlobalTransform().getMatrix();
          pl::ShadowPipelineLayout::PushConstants pcs{
              .model = model,
              .primIndex = prim->getStorageId().value(),
              .skinOffset = asset_->skins.getOffsetForNode(node),
              .enableSkinning =
                  (node->skin != nullptr && enableSkinning_) ? 1U : 0U,
              ._pad = 0U,
          };

          cb.pushConstants(layout_, vk::ShaderStageFlagBits::eVertex, 0,
                           sizeof(pcs), &pcs);
          cb.bindIndexBuffer(prim->getIndexBuffer()->buffer,
                             prim->getIndexBufferOffset(),
                             prim->getIndexType());
          cb.drawIndexed(prim->attrs.index.info.count, 1, 0, 0, 0);
        }
      }
      for (const auto& child : node->getChildren()) self(self, child.get());
    };

    for (std::uint32_t root_id : active_scene->rootNodes) {
      if (auto node = asset_->nodes.get(root_id))
        traverse_node(traverse_node, node.get());
    }
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet lightSet_;
  vk::DescriptorSet primSet_;
  const asset::Asset* asset_;
  const material::MaterialManager* materialManager_;
  bool enableSkinning_;
};

};  // namespace vkit::renderer::cmd
