#pragma once

#include <functional>
#include <string>
#include <vector>

#include "vkit/asset/material_info.hpp"

#include "vkit/compute/compute_output_dispatcher.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/execution_context.hpp"
#include "vkit/workflow/node/material/diffuse.hpp"
#include "vkit/workflow/node/material/diffuse_specular.hpp"
#include "vkit/workflow/node/material/mix_material.hpp"
#include "vkit/workflow/node/material/principled_bsdf.hpp"
#include "vkit/workflow/node/material/slot_output.hpp"
#include "vkit/workflow/node/operators/heightmap.hpp"
#include "vkit/workflow/node/operators/mix.hpp"
#include "vkit/workflow/node/operators/normalmap.hpp"
#include "vkit/workflow/node/operators/sobel.hpp"
#include "vkit/workflow/node/operators/channel_adjust.hpp"
#include "vkit/workflow/node/operators/channel_remap.hpp"
#include "vkit/workflow/node/operators/tint.hpp"
#include "vkit/workflow/node/procedural/fractal_generator.hpp"
#include "vkit/workflow/node/procedural/noise_generator.hpp"
#include "vkit/workflow/node/procedural/pattern_generator.hpp"
#include "vkit/workflow/node/texture_load.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::controller {

struct NodePosition {
  int nodeId;
  float x;
  float y;
};

class WorkflowController {
 public:
  WorkflowController() = default;

  auto setWorkflow(workflow::Workflow* w) -> WorkflowController&;
  auto setTextureManager(texture::TextureManager* m) -> WorkflowController&;
  auto setMaterialManager(material::MaterialManager* m) -> WorkflowController&;
  auto setExecutionContext(workflow::ExecutionContext* ctx)
      -> WorkflowController&;
  auto setComputeDispatcher(compute::ComputeOutputDispatcher* d)
      -> WorkflowController&;

  auto createTextureLoadNode(const std::string& name = "Texture Load")
      -> workflow::node::TextureLoadNode*;
  auto createFractalGeneratorNode(const std::string& name = "Fractal Generator")
      -> workflow::node::proc::FractalGeneratorNode*;
  auto createNoiseGeneratorNode(const std::string& name = "Noise Generator")
      -> workflow::node::proc::NoiseGeneratorNode*;
  auto createPatternGeneratorNode(const std::string& name = "Pattern Generator")
      -> workflow::node::proc::PatternGeneratorNode*;
  auto createSobelNode(const std::string& name = "Sobel Edge")
      -> workflow::node::op::SobelNode*;
  auto createHeightMapNode(const std::string& name = "Height Map")
      -> workflow::node::op::HeightMapNode*;
  auto createNormalMapNode(const std::string& name = "Normal Map")
      -> workflow::node::op::NormalMapNode*;
  auto createTintNode(const std::string& name = "Color Tint")
      -> workflow::node::op::TintNode*;
  auto createChannelRemapNode(const std::string& name = "Channel Remap")
      -> workflow::node::op::ChannelRemapNode*;
  auto createChannelAdjustNode(const std::string& name = "Channel Adjust")
      -> workflow::node::op::ChannelAdjustNode*;
  auto createMixNode(const std::string& name = "Mix")
      -> workflow::node::op::MixNode*;
  auto createDiffuseNode(const std::string& name = "Diffuse")
      -> workflow::node::mat::DiffuseNode*;
  auto createDiffuseSpecularNode(const std::string& name = "Diffuse Specular")
      -> workflow::node::mat::DiffuseSpecularNode*;
  auto createPrincipledBSDFNode(const std::string& name = "Principled BSDF")
      -> workflow::node::mat::PrincipledBSDFNode*;
  auto createMixMaterialNode(const std::string& name = "Mix Material")
      -> workflow::node::mat::MixMaterialNode*;
  auto createSlotOutputNode(const std::string& name = "Material Slot")
      -> workflow::node::mat::SlotOutputNode*;

  void setDeviceWaitFn(std::function<void()> fn) {
    deviceWaitFn_ = std::move(fn);
  }

  void deleteNodes(const std::vector<int>& nodeIds);
  [[nodiscard]] auto canConnectPins(int a, int b) const -> bool;
  void connectPins(int src, int dst);
  void disconnectLink(int linkId);
  void clearWorkflow();
  void pushPendingPositions(std::vector<NodePosition> positions);
  [[nodiscard]] auto getWorkflow() const -> workflow::Workflow* {
    return workflow_;
  }

  void importAssetMaterials(
      const std::vector<asset::GltfMaterialInfo>& materials);
  [[nodiscard]] auto drainPendingPositions() -> std::vector<NodePosition>;

 private:
  workflow::Workflow* workflow_{nullptr};
  texture::TextureManager* textureManager_{nullptr};
  material::MaterialManager* materialManager_{nullptr};
  workflow::ExecutionContext* ctx_{nullptr};
  compute::ComputeOutputDispatcher* dispatcher_{nullptr};
  std::function<void()> deviceWaitFn_;

  std::vector<NodePosition> pendingPositions_;
};

}  // namespace vkit::controller
