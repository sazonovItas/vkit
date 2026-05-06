#include "vkit/controller/workflow.hpp"

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

}  // namespace vkit::controller
