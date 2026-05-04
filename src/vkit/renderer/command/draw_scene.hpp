#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/graphics/command.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/renderer/pipeline_layout/primitive_material.hpp"
#include "vkit/renderer/pipeline_layout/ray_sphere_material.hpp"
#include "vkit/renderer/pipeline_layout/skybox.hpp"

namespace vkit::renderer::cmd {

class DrawSkyboxCommand final : public graphics::Command {
 public:
  DrawSkyboxCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                    vk::DescriptorSet sceneSet, vk::DescriptorSet bindlessSet,
                    glm::vec4 baseColor, float blurLevel)
      : pipeline_{pipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        bindlessSet_{bindlessSet},
        baseColor_{baseColor},
        blurLevel_{blurLevel} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
    std::array<vk::DescriptorSet, 2> sets = {sceneSet_, bindlessSet_};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    auto pcs = pl::SkyboxPipelineLayout::PushConstants{
        .baseColor = baseColor_,
        .blurLevel = blurLevel_,
    };
    cb.pushConstants(layout_, vk::ShaderStageFlagBits::eFragment, 0,
                     sizeof(pcs), &pcs);
    cb.draw(6, 1, 0, 0);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  vk::DescriptorSet bindlessSet_;
  glm::vec4 baseColor_;
  float blurLevel_;
};

class DrawRaySphereCommand final : public graphics::Command {
 public:
  DrawRaySphereCommand(
      vk::Pipeline opaquePipeline,
      vk::Pipeline transparentPipeline,  // <-- Added Transparent
      vk::PipelineLayout layout, vk::DescriptorSet sceneSet,
      vk::DescriptorSet bindlessSet, vk::DescriptorSet materialSet,
      glm::mat4 model, std::uint32_t materialSlot,
      const material::MaterialManager* materialManager)
      : opaquePipeline_{opaquePipeline},
        transparentPipeline_{transparentPipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        bindlessSet_{bindlessSet},
        materialSet_{materialSet},
        model_{model},
        materialSlot_{materialSlot},
        materialManager_{materialManager} {}

  void record(vk::CommandBuffer cb) const override {
    std::uint32_t mat_type = 0;
    std::uint32_t mat_idx = 0;
    vk::Pipeline active_pipeline = opaquePipeline_;

    if (materialManager_) {
      auto slot = materialManager_->getSlot(materialSlot_);

      if (slot && slot->getMaterialType() != material::Type::kNone) {
        auto m_type = slot->getMaterialType();
        auto m_id = slot->getMaterialId();

        auto mat = materialManager_->getMaterial(m_type, m_id);
        if (mat) {
          mat_type = static_cast<std::uint32_t>(m_type);
          mat_idx = m_id;

          if (mat->getAlphaMode() == material::AlphaMode::kBlend) {
            active_pipeline = transparentPipeline_;
          }
        }
      }
    }

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, active_pipeline);

    std::array<vk::DescriptorSet, 3> sets = {sceneSet_, bindlessSet_,
                                             materialSet_};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    auto pcs = pl::RaySphereMaterialPipelineLayout::PushConstants{
        .model = model_,
        .materialType = mat_type,
        .materialIndex = mat_idx,
    };

    cb.pushConstants(
        layout_,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(pcs), &pcs);
    cb.draw(36, 1, 0, 0);
  }

 private:
  vk::Pipeline opaquePipeline_;
  vk::Pipeline transparentPipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  vk::DescriptorSet bindlessSet_;
  vk::DescriptorSet materialSet_;
  glm::mat4 model_;
  std::uint32_t materialSlot_;
  const material::MaterialManager* materialManager_;
};

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
                   const material::MaterialManager* materialManager,
                   bool enableSkinning)
      : opaquePipeline_{opaquePipeline},
        transparentPipeline_{transparentPipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        bindlessSet_{bindlessSet},
        materialSet_{materialSet},
        primSet_{primSet},
        asset_{asset},
        materialManager_{materialManager},
        enableSkinning_{enableSkinning} {}

  void record(vk::CommandBuffer cb) const override {
    if (!asset_ || asset_->scenes.empty() || !materialManager_) return;
    auto active_scene = asset_->getActiveScene();
    if (!active_scene) return;

    std::vector<PrimitiveDrawInfo> opaque_draws;
    std::vector<PrimitiveDrawInfo> transparent_draws;

    auto traverse_node = [&](auto& self, const scene::Node* node) -> void {
      if (node->mesh) {
        for (const auto& prim : node->mesh->getPrimitives()) {
          if (!prim->attrs.index.isValid() || !prim->getStorageId().has_value())
            continue;

          std::uint32_t final_mat_type = 0;
          std::uint32_t final_mat_index = 0;
          material::AlphaMode alpha_mode = material::AlphaMode::kOpaque;

          auto slot = materialManager_->getSlot(prim->getMaterialSlot());

          if (slot && slot->getMaterialType() != material::Type::kNone) {
            auto m_type = slot->getMaterialType();
            auto m_id = slot->getMaterialId();

            auto mat = materialManager_->getMaterial(m_type, m_id);
            if (mat) {
              alpha_mode = mat->getAlphaMode();
              final_mat_type = static_cast<std::uint32_t>(m_type);
              final_mat_index = m_id;
            }
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
                  .enableSkinning =
                      (node->skin != nullptr && enableSkinning_) ? 1U : 0U,
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
      for (const auto& child : node->getChildren()) self(self, child.get());
    };

    for (std::uint32_t root_id : active_scene->rootNodes) {
      if (auto node = asset_->nodes.get(root_id))
        traverse_node(traverse_node, node.get());
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
  const bool enableSkinning_;
};

};  // namespace vkit::renderer::cmd
