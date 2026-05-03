#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/material/material.hpp"
#include "vkit/renderer/pipeline_layout/primitive_material.hpp"

namespace vkit::renderer::cmd {

class DrawAssetCommand final : public graphics::Command {
 private:
  struct PrimitiveDrawInfo {
    vk::Buffer indexBuffer;
    vk::DeviceSize indexOffset;
    vk::IndexType indexType;
    std::uint32_t indexCount;
    pl::PrimitiveMaterialPipelineLayout::PushConstants pcs;
  };

 public:
  DrawAssetCommand(vk::Pipeline opaquePipeline,
                   vk::Pipeline transparentPipeline, vk::PipelineLayout layout,
                   vk::DescriptorSet sceneSet, vk::DescriptorSet bindlessSet,
                   vk::DescriptorSet materialSet, vk::DescriptorSet primSet,
                   const asset::Asset* asset,
                   const material::MaterialManager* materialManager)
      : opaquePipeline_{opaquePipeline},
        transparentPipeline_{transparentPipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        bindlessSet_{bindlessSet},
        materialSet_{materialSet},
        primSet_{primSet},
        asset_{asset},
        materialManager_{materialManager} {}

  void record(vk::CommandBuffer cb) const override {
    if (!asset_ || asset_->scenes.empty() || !materialManager_) return;

    auto active_scene = asset_->getActiveScene();
    if (!active_scene) return;

    std::vector<PrimitiveDrawInfo> opaque_draws;
    std::vector<PrimitiveDrawInfo> transparent_draws;

    auto traverse_node = [&](auto& self, const scene::Node* node) -> void {
      if (node->mesh) {
        for (const auto& prim : node->mesh->getPrimitives()) {
          if (!prim->attrs.index.isValid() ||
              !prim->getStorageId().has_value()) {
            continue;
          }

          auto mat = materialManager_->getMaterial(material::Type::kNone, 0);

          auto final_mat_type =
              static_cast<std::uint32_t>(material::Type::kNone);
          std::uint32_t final_mat_index = 0;
          material::AlphaMode alpha_mode = material::AlphaMode::kOpaque;

          if (mat) {
            final_mat_type = static_cast<std::uint32_t>(mat->getType());
            final_mat_index = mat->getStorageId().value_or(0);
            alpha_mode = mat->getAlphaMode();
          }

          PrimitiveDrawInfo draw_info{
              .indexBuffer = prim->getIndexBuffer()->buffer,
              .indexOffset = prim->getIndexBufferOffset(),
              .indexType = prim->getIndexType(),
              .indexCount = prim->attrs.index.info.count,
              .pcs = {
                  .model = node->getGlobalTransform().getMatrix(),
                  .primIndex = prim->getStorageId().value(),
                  .skinOffset = asset_->skins.getOffsetForNode(node),
                  .enableSkinning = (node->skin != nullptr) ? 1U : 0U,
                  .materialType = final_mat_type,
                  .materialIndex = final_mat_index,
              }};

          if (alpha_mode == material::AlphaMode::kBlend) {
            transparent_draws.push_back(draw_info);
          } else {
            opaque_draws.push_back(draw_info);
          }
        }
      }

      for (const auto& child : node->getChildren()) {
        self(self, child.get());
      }
    };

    for (std::uint32_t root_id : active_scene->rootNodes) {
      if (auto node = asset_->nodes.get(root_id)) {
        traverse_node(traverse_node, node.get());
      }
    }

    std::array<vk::DescriptorSet, 4> sets = {sceneSet_, bindlessSet_,
                                             materialSet_, primSet_};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    if (!opaque_draws.empty()) {
      cb.bindPipeline(vk::PipelineBindPoint::eGraphics, opaquePipeline_);
      for (const auto& draw : opaque_draws) {
        cb.bindIndexBuffer(draw.indexBuffer, draw.indexOffset, draw.indexType);
        cb.pushConstants(layout_,
                         vk::ShaderStageFlagBits::eVertex |
                             vk::ShaderStageFlagBits::eFragment,
                         0, sizeof(draw.pcs), &draw.pcs);
        cb.drawIndexed(draw.indexCount, 1, 0, 0, 0);
      }
    }

    if (!transparent_draws.empty()) {
      cb.bindPipeline(vk::PipelineBindPoint::eGraphics, transparentPipeline_);
      for (const auto& draw : transparent_draws) {
        cb.bindIndexBuffer(draw.indexBuffer, draw.indexOffset, draw.indexType);
        cb.pushConstants(layout_,
                         vk::ShaderStageFlagBits::eVertex |
                             vk::ShaderStageFlagBits::eFragment,
                         0, sizeof(draw.pcs), &draw.pcs);
        cb.drawIndexed(draw.indexCount, 1, 0, 0, 0);
      }
    }
  }

 private:
  vk::Pipeline opaquePipeline_;
  vk::Pipeline transparentPipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  vk::DescriptorSet bindlessSet_;
  vk::DescriptorSet materialSet_;
  vk::DescriptorSet primSet_;
  const asset::Asset* asset_;
  const material::MaterialManager* materialManager_;
};

};  // namespace vkit::renderer::cmd
