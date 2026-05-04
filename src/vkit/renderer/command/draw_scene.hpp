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
    std::array<vk::DescriptorSet, 2> sets = {
        sceneSet_,
        bindlessSet_,
    };
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
  DrawRaySphereCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                       vk::DescriptorSet sceneSet,
                       vk::DescriptorSet bindlessSet,
                       vk::DescriptorSet materialSet, glm::mat4 model,
                       std::uint32_t materialType = 0,
                       std::uint32_t materialIndex = 0)
      : pipeline_{pipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        bindlessSet_{bindlessSet},
        materialSet_{materialSet},
        model_{model},
        materialType_{materialType},
        materialIndex_{materialIndex} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

    std::array<vk::DescriptorSet, 3> sets = {
        sceneSet_,
        bindlessSet_,
        materialSet_,
    };
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    auto pcs = pl::RaySphereMaterialPipelineLayout::PushConstants{
        .model = model_,
        .materialType = materialType_,
        .materialIndex = materialIndex_,
    };

    cb.pushConstants(
        layout_,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(pcs), &pcs);
    cb.draw(36, 1, 0, 0);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  vk::DescriptorSet bindlessSet_;
  vk::DescriptorSet materialSet_;
  glm::mat4 model_;
  std::uint32_t materialType_;
  std::uint32_t materialIndex_;
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
          if (!prim->attrs.index.isValid() || !prim->getStorageId().has_value())
            continue;

          auto mat = materialManager_->getMaterial(prim->getMaterialType(),
                                                   prim->getMaterialId());
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
      for (const auto& child : node->getChildren()) self(self, child.get());
    };

    for (std::uint32_t root_id : active_scene->rootNodes) {
      if (auto node = asset_->nodes.get(root_id))
        traverse_node(traverse_node, node.get());
    }

    std::array<vk::DescriptorSet, 4> sets = {
        sceneSet_,
        bindlessSet_,
        materialSet_,
        primSet_,
    };
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
