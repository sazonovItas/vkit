#include "vkit/controller/workflow.hpp"

#include <algorithm>
#include <filesystem>

#include "vkit/material/material.hpp"

namespace vkit::controller {

auto WorkflowController::setWorkflow(workflow::Workflow* w)
    -> WorkflowController& {
  workflow_ = w;
  return *this;
}
auto WorkflowController::setTextureManager(texture::TextureManager* m)
    -> WorkflowController& {
  textureManager_ = m;
  return *this;
}
auto WorkflowController::setMaterialManager(material::MaterialManager* m)
    -> WorkflowController& {
  materialManager_ = m;
  return *this;
}
auto WorkflowController::setExecutionContext(workflow::ExecutionContext* ctx)
    -> WorkflowController& {
  ctx_ = ctx;
  return *this;
}
auto WorkflowController::setComputeDispatcher(
    compute::ComputeOutputDispatcher* d) -> WorkflowController& {
  dispatcher_ = d;
  return *this;
}

auto WorkflowController::createTextureLoadNode(const std::string& name)
    -> workflow::node::TextureLoadNode* {
  if (!workflow_ || !ctx_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::TextureLoadNode>(
      name, ctx_->texLoadBus, ctx_->texReadyBus, *textureManager_);
}

auto WorkflowController::createFractalGeneratorNode(const std::string& name)
    -> workflow::node::proc::FractalGeneratorNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::proc::FractalGeneratorNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("fractal"));
}

auto WorkflowController::createNoiseGeneratorNode(const std::string& name)
    -> workflow::node::proc::NoiseGeneratorNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::proc::NoiseGeneratorNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("noise"));
}

auto WorkflowController::createPatternGeneratorNode(const std::string& name)
    -> workflow::node::proc::PatternGeneratorNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::proc::PatternGeneratorNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("pattern"));
}

auto WorkflowController::createSobelNode(const std::string& name)
    -> workflow::node::op::SobelNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::op::SobelNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("sobel"));
}

auto WorkflowController::createHeightMapNode(const std::string& name)
    -> workflow::node::op::HeightMapNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::op::HeightMapNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("heightmap"));
}

auto WorkflowController::createNormalMapNode(const std::string& name)
    -> workflow::node::op::NormalMapNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::op::NormalMapNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("normalmap"));
}

auto WorkflowController::createTintNode(const std::string& name)
    -> workflow::node::op::TintNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::op::TintNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("tint"));
}

auto WorkflowController::createMixNode(const std::string& name)
    -> workflow::node::op::MixNode* {
  if (!workflow_ || !ctx_ || !dispatcher_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::op::MixNode>(
      name, *textureManager_, ctx_->computeOutputBus,
      ctx_->computeOutputResultBus, dispatcher_->getPipeline("mix"));
}

auto WorkflowController::createPrincipledBSDFNode(const std::string& name)
    -> workflow::node::mat::PrincipledBSDFNode* {
  if (!workflow_ || !materialManager_ || !textureManager_) return nullptr;
  return workflow_->createNode<workflow::node::mat::PrincipledBSDFNode>(
      name, *materialManager_, *textureManager_);
}

auto WorkflowController::createSlotOutputNode(const std::string& name)
    -> workflow::node::mat::SlotOutputNode* {
  if (!workflow_ || !materialManager_) return nullptr;
  return workflow_->createNode<workflow::node::mat::SlotOutputNode>(
      name, *materialManager_);
}

void WorkflowController::deleteNodes(const std::vector<int>& nodeIds) {
  if (!workflow_) return;
  for (int id : nodeIds) workflow_->destroyNode(id);
}

auto WorkflowController::canConnectPins(int a, int b) const -> bool {
  if (!workflow_) return false;
  auto* pa = workflow_->findPin(a);
  auto* pb = workflow_->findPin(b);
  return (pa != nullptr) && (pb != nullptr) && workflow_->canConnect(pa, pb);
}

void WorkflowController::connectPins(int src, int dst) {
  if (!workflow_) return;
  auto* ps = workflow_->findPin(src);
  auto* pd = workflow_->findPin(dst);
  if (ps && pd && workflow_->canConnect(ps, pd)) {
    workflow_->connect(ps, pd);
    workflow_->markDirty();
  }
}

void WorkflowController::disconnectLink(int linkId) {
  if (!workflow_) return;
  auto* link = workflow_->findLink(linkId);
  if (link) {
    workflow_->disconnect(link);
    workflow_->markDirty();
  }
}

auto WorkflowController::drainPendingPositions() -> std::vector<NodePosition> {
  std::vector<NodePosition> result;
  std::swap(result, pendingPositions_);
  return result;
}

void WorkflowController::importAssetMaterials(
    const std::vector<asset::GltfMaterialInfo>& materials) {
  if (materials.empty() || !workflow_) return;

  // Layout constants (editor grid units ≈ screen pixels at 1:1 zoom)
  constexpr float kStartX = 80.0f;
  constexpr float kStartY = 80.0f;
  constexpr float kTexNodeW = 210.0f;
  constexpr float kTexNodeH = 88.0f;
  constexpr float kTexVGap = 16.0f;
  constexpr float kBSDFNodeW = 240.0f;
  constexpr float kSlotNodeW = 200.0f;
  constexpr float kHGap = 56.0f;
  constexpr float kColGap = 80.0f;
  constexpr float kColWidth =
      kTexNodeW + kHGap + kBSDFNodeW + kHGap + kSlotNodeW + kColGap;
  // PrincipledBSDF pin indices — must match principled_bsdf.cpp addInputPin order
  // 0:Base Color, 1:Normal, 2:Met/Rough, 3:Emissive, 4:Occlusion,
  // 5:Specular, 6:Specular Color, 7:Thickness, 8:Clearcoat,
  // 9:Clearcoat Normal, 10:Sheen Color, 11:Sheen Roughness,
  // 12:Anisotropy, 13:Iridescence, 14:Irid.Thickness
  struct TexBinding {
    const asset::GltfTextureRef* ref;
    int bsdfPinIdx;
    const char* label;
  };

  for (std::size_t m = 0; m < materials.size(); ++m) {
    const auto& mat = materials[m];
    const float colX = kStartX + static_cast<float>(m) * kColWidth;

    const TexBinding kAllBindings[] = {
        {mat.baseColorTexture ? &*mat.baseColorTexture : nullptr,           0, "Base Color"},
        {mat.normalTexture ? &*mat.normalTexture : nullptr,                 1, "Normal"},
        {mat.metallicRoughnessTexture ? &*mat.metallicRoughnessTexture : nullptr, 2, "Met/Rough"},
        {mat.emissiveTexture ? &*mat.emissiveTexture : nullptr,             3, "Emissive"},
        {mat.occlusionTexture ? &*mat.occlusionTexture : nullptr,           4, "Occlusion"},
        {mat.specularTexture ? &*mat.specularTexture : nullptr,             5, "Specular"},
        {mat.specularColorTexture ? &*mat.specularColorTexture : nullptr,   6, "Spec.Color"},
        {mat.thicknessTexture ? &*mat.thicknessTexture : nullptr,           7, "Thickness"},
        {mat.clearcoatTexture ? &*mat.clearcoatTexture : nullptr,           8, "Clearcoat"},
        {mat.clearcoatNormalTexture ? &*mat.clearcoatNormalTexture : nullptr, 9, "CC Normal"},
        {mat.sheenColorTexture ? &*mat.sheenColorTexture : nullptr,        10, "Sheen Color"},
        {mat.sheenRoughnessTexture ? &*mat.sheenRoughnessTexture : nullptr,11, "Sheen Rough"},
        {mat.anisotropyTexture ? &*mat.anisotropyTexture : nullptr,        12, "Anisotropy"},
        {mat.iridescenceTexture ? &*mat.iridescenceTexture : nullptr,      13, "Iridescence"},
        {mat.iridescenceThicknessTexture ? &*mat.iridescenceThicknessTexture : nullptr, 14, "Irid.Thick"},
    };

    // Build active bindings (only slots that have a texture)
    std::vector<const TexBinding*> active;
    for (const auto& b : kAllBindings)
      if (b.ref) active.push_back(&b);

    // Create and position texture load nodes
    std::vector<workflow::node::TextureLoadNode*> texNodes;
    texNodes.reserve(active.size());
    for (std::size_t i = 0; i < active.size(); ++i) {
      std::string nodeName = mat.name.empty()
                                 ? std::string{active[i]->label}
                                 : mat.name + " \xe2\x80\x93 " + active[i]->label;
      auto* tex = createTextureLoadNode(nodeName);
      if (!tex) continue;
      tex->setPath(active[i]->ref->path);

      float ty = kStartY + static_cast<float>(i) * (kTexNodeH + kTexVGap);
      pendingPositions_.push_back({tex->getId(), colX, ty});
      texNodes.push_back(tex);
    }

    // Vertical centre for BSDF relative to the tex-node column
    float totalTexH =
        active.empty() ? 0.0f
                       : static_cast<float>(active.size() - 1) *
                                 (kTexNodeH + kTexVGap) +
                             kTexNodeH;
    constexpr float kBSDFHalfH = 180.0f;
    float bsdfY = active.empty()
                      ? kStartY
                      : std::max(kStartY, kStartY + totalTexH / 2.0f - kBSDFHalfH);

    // Create PrincipledBSDF node and apply all scalar parameters
    std::string bsdfName = mat.name.empty() ? "Principled BSDF" : mat.name;
    auto* bsdf = createPrincipledBSDFNode(bsdfName);
    if (!bsdf) continue;

    bsdf->alphaMode              = static_cast<material::AlphaMode>(mat.alphaMode);
    bsdf->baseColorFactor        = mat.baseColorFactor;
    bsdf->metallicFactor         = mat.metallicFactor;
    bsdf->roughnessFactor        = mat.roughnessFactor;
    bsdf->emissiveFactor         = mat.emissiveFactor;
    bsdf->occlusionStrength      = mat.occlusionStrength;
    bsdf->ior                    = mat.ior;
    bsdf->specularFactor         = mat.specularFactor;
    bsdf->specularColorFactor    = mat.specularColorFactor;
    bsdf->transmissionFactor     = mat.transmissionFactor;
    bsdf->thicknessFactor        = mat.thicknessFactor;
    bsdf->attenuationDistance    = mat.attenuationDistance;
    bsdf->attenuationColor       = mat.attenuationColor;
    bsdf->sheenColorFactor       = mat.sheenColorFactor;
    bsdf->sheenRoughnessFactor   = mat.sheenRoughnessFactor;
    bsdf->clearcoatFactor        = mat.clearcoatFactor;
    bsdf->clearcoatRoughnessFactor = mat.clearcoatRoughnessFactor;
    bsdf->anisotropyStrength     = mat.anisotropyStrength;
    bsdf->anisotropyRotation     = mat.anisotropyRotation;
    bsdf->iridescenceFactor      = mat.iridescenceFactor;
    bsdf->iridescenceIor         = mat.iridescenceIor;
    bsdf->iridescenceThicknessMin = mat.iridescenceThicknessMin;
    bsdf->iridescenceThicknessMax = mat.iridescenceThicknessMax;

    float bsdfX = colX + kTexNodeW + kHGap;
    pendingPositions_.push_back({bsdf->getId(), bsdfX, bsdfY});

    // Wire each texture node's Color output → matching BSDF input pin
    auto& bsdfInputs = bsdf->getInputs();
    for (std::size_t i = 0; i < texNodes.size(); ++i) {
      auto& texOutputs = texNodes[i]->getOutputs();
      int pinIdx = active[i]->bsdfPinIdx;
      if (texOutputs.size() > 1 &&
          static_cast<std::size_t>(pinIdx) < bsdfInputs.size()) {
        connectPins(texOutputs[1]->getId(), bsdfInputs[pinIdx]->getId());
      }
    }

    // Create and position SlotOutput node
    std::string slotName =
        mat.name.empty() ? "Slot " + std::to_string(m) : mat.name + " Slot";
    auto* slot = createSlotOutputNode(slotName);
    if (!slot) continue;
    slot->targetSlotId = static_cast<std::uint32_t>(m);

    float slotX = bsdfX + kBSDFNodeW + kHGap;
    float slotY = bsdfY + kBSDFHalfH - 40.0f;
    pendingPositions_.push_back({slot->getId(), slotX, slotY});

    // Wire BSDF Material output → SlotOutput input
    auto& bsdfOutputs = bsdf->getOutputs();
    auto& slotInputs  = slot->getInputs();
    if (!bsdfOutputs.empty() && !slotInputs.empty()) {
      connectPins(bsdfOutputs[0]->getId(), slotInputs[0]->getId());
    }
  }
}

}  // namespace vkit::controller
