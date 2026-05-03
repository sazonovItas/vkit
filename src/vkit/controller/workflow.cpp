#include "vkit/controller/workflow.hpp"

namespace vkit::controller {

auto WorkflowController::setWorkflow(workflow::Workflow* workflow)
    -> WorkflowController& {
  workflow_ = workflow;
  return *this;
}

auto WorkflowController::setTextureManager(
    texture::TextureManager* textureManager) -> WorkflowController& {
  textureManager_ = textureManager;
  return *this;
}

auto WorkflowController::setTextureLoadBus(core::events::TextureLoadBus* bus)
    -> WorkflowController& {
  textureLoadBus_ = bus;
  return *this;
}

auto WorkflowController::setTextureReadyBus(core::events::TextureReadyBus* bus)
    -> WorkflowController& {
  textureReadyBus_ = bus;
  return *this;
}

auto WorkflowController::setNoiseJobBus(core::events::NoiseJobBus* bus)
    -> WorkflowController& {
  noiseJobBus_ = bus;
  return *this;
}

auto WorkflowController::setNoiseResultBus(core::events::NoiseResultBus* bus)
    -> WorkflowController& {
  noiseResultBus_ = bus;
  return *this;
}

auto WorkflowController::setSobelJobBus(core::events::SobelJobBus* bus)
    -> WorkflowController& {
  sobelJobBus_ = bus;
  return *this;
}

auto WorkflowController::setSobelResultBus(core::events::SobelResultBus* bus)
    -> WorkflowController& {
  sobelResultBus_ = bus;
  return *this;
}

auto WorkflowController::setHeightMapJobBus(core::events::HeightMapJobBus* bus)
    -> WorkflowController& {
  heightMapJobBus_ = bus;
  return *this;
}

auto WorkflowController::setHeightMapResultBus(
    core::events::HeightMapResultBus* bus) -> WorkflowController& {
  heightMapResultBus_ = bus;
  return *this;
}

auto WorkflowController::setTintJobBus(core::events::TintJobBus* bus)
    -> WorkflowController& {
  tintJobBus_ = bus;
  return *this;
}

auto WorkflowController::setTintResultBus(core::events::TintResultBus* bus)
    -> WorkflowController& {
  tintResultBus_ = bus;
  return *this;
}

auto WorkflowController::createTextureLoadNode(const std::string& name)
    -> workflow::node::TextureLoadNode* {
  if (!workflow_ || !textureLoadBus_ || !textureReadyBus_ || !textureManager_)
    return nullptr;

  return workflow_->createNode<workflow::node::TextureLoadNode>(
      name, *textureLoadBus_, *textureReadyBus_, *textureManager_);
}

auto WorkflowController::createNoiseGeneratorNode(const std::string& name)
    -> workflow::node::NoiseGeneratorNode* {
  if (!workflow_ || !noiseJobBus_ || !noiseResultBus_ || !textureManager_)
    return nullptr;

  return workflow_->createNode<workflow::node::NoiseGeneratorNode>(
      name, *noiseJobBus_, *noiseResultBus_, *textureManager_);
}

auto WorkflowController::createSobelNode(const std::string& name)
    -> workflow::node::SobelNode* {
  if (!workflow_ || !sobelJobBus_ || !sobelResultBus_ || !textureManager_)
    return nullptr;

  return workflow_->createNode<workflow::node::SobelNode>(
      name, *sobelJobBus_, *sobelResultBus_, *textureManager_);
}

auto WorkflowController::createHeightMapNode(const std::string& name)
    -> workflow::node::HeightMapNode* {
  if (!workflow_ || !heightMapJobBus_ || !heightMapResultBus_ ||
      !textureManager_)
    return nullptr;

  return workflow_->createNode<workflow::node::HeightMapNode>(
      name, *heightMapJobBus_, *heightMapResultBus_, *textureManager_);
}

auto WorkflowController::createTintNode(const std::string& name)
    -> workflow::node::TintNode* {
  if (!workflow_ || !tintJobBus_ || !tintResultBus_ || !textureManager_)
    return nullptr;

  return workflow_->createNode<workflow::node::TintNode>(
      name, *tintJobBus_, *tintResultBus_, *textureManager_);
}

void WorkflowController::deleteNodes(const std::vector<int>& nodeIds) {
  if (!workflow_) return;
  for (int id : nodeIds) {
    workflow_->destroyNode(id);
  }
}

auto WorkflowController::canConnectPins(int pinIdA, int pinIdB) const -> bool {
  if (!workflow_) return false;

  auto* pin_a = workflow_->findPin(pinIdA);
  auto* pin_b = workflow_->findPin(pinIdB);

  if (!pin_a || !pin_b) return false;

  return workflow_->canConnect(pin_a, pin_b);
}

void WorkflowController::connectPins(int sourcePinId, int targetPinId) {
  if (!workflow_) return;

  auto* src = workflow_->findPin(sourcePinId);
  auto* dst = workflow_->findPin(targetPinId);

  if (src && dst && workflow_->canConnect(src, dst)) {
    workflow_->connect(src, dst);
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

};  // namespace vkit::controller
