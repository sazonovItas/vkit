#include "vkit/workflow/node/material/diffuse.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/material/diffuse.hpp"
#include "vkit/workflow/node/material/material_link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::mat {

DiffuseNode::DiffuseNode(std::string_view name,
                         material::MaterialManager& matManager,
                         texture::TextureManager& texManager)
    : WorkflowNode(name), matManager_(matManager), texManager_(texManager) {
  auto mat = std::make_shared<material::Diffuse>("Node_Diffuse");
  managerId_ = matManager_.addMaterial(mat);

  inDiffuseTex_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Diffuse");
  inNormalTex_  = addInputPin(pinKeyType(PinType::kColorTexture2D), "Normal Map");

  outMaterial_ = addOutputPin(pinKeyType(PinType::kMaterial), "Material");

  MaterialLink link{.type = material::Type::kDiffuse, .managerId = managerId_};
  outMaterial_->setData<MaterialLink>(link);

  onStateChanged = [this]() {
    if (status_ != NodeStatus::kStale) return;
    auto m = matManager_.getMaterial(material::Type::kDiffuse, managerId_);
    if (!m) return;
    auto diff = std::static_pointer_cast<material::Diffuse>(m);

    bool changed = false;
    auto strip_loading_tex = [&](graph::Pin* pin, auto clear_func) {
      if (!pin) return;
      const auto& links = pin->getLinks();
      if (!links.empty()) {
        auto* src_node = static_cast<workflow::WorkflowNode*>(
            links.front()->getSrc()->getOwnerNode());
        if (src_node->status() != NodeStatus::kReady) {
          clear_func();
          changed = true;
        }
      }
    };

    strip_loading_tex(inDiffuseTex_, [&]() { diff->setDiffuseTexture(nullptr); });
    strip_loading_tex(inNormalTex_,  [&]() { diff->setNormalTexture(nullptr); });

    if (changed) diff->setDirty(true);
  };
}

DiffuseNode::~DiffuseNode() {
  matManager_.removeMaterial(material::Type::kDiffuse, managerId_);
}

void DiffuseNode::execute() {
  auto mat = matManager_.getMaterial(material::Type::kDiffuse, managerId_);
  if (!mat) {
    setStatus(NodeStatus::kError);
    return;
  }

  auto diff = std::static_pointer_cast<material::Diffuse>(mat);
  diff->setAlphaMode(alphaMode);
  diff->params.diffuseFactor = diffuseFactor;

  bool waiting = false;
  auto resolve_tex = [&](graph::Pin* pin, auto setter) {
    if (pin->hasData()) {
      setter(texManager_.get(*pin->getData<std::uint32_t>()));
      return;
    }
    const auto& links = pin->getLinks();
    if (links.empty()) { setter(nullptr); return; }
    auto* src_pin  = links.front()->getSrc();
    auto* src_node = static_cast<workflow::WorkflowNode*>(src_pin->getOwnerNode());
    if (src_node->status() != NodeStatus::kReady) { waiting = true; setter(nullptr); return; }
    if (const auto* id = src_pin->getData<std::uint32_t>())
      setter(texManager_.get(*id));
    else
      setter(nullptr);
  };

  resolve_tex(inDiffuseTex_, [&](const auto& t) { diff->setDiffuseTexture(t); });
  resolve_tex(inNormalTex_,  [&](const auto& t) { diff->setNormalTexture(t); });

  diff->setDirty(true);

  if (waiting) {
    setStatus(NodeStatus::kStale);
    propagateStale();
  } else {
    setStatus(NodeStatus::kReady);
  }
}

};  // namespace vkit::workflow::node::mat
