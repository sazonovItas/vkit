#include "vkit/workflow/node/material/diffuse_specular.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/material/diffuse_specular.hpp"
#include "vkit/workflow/node/material/material_link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::mat {

DiffuseSpecularNode::DiffuseSpecularNode(std::string_view name,
                                         material::MaterialManager& matManager,
                                         texture::TextureManager& texManager)
    : WorkflowNode(name), matManager_(matManager), texManager_(texManager) {
  auto mat = std::make_shared<material::DiffuseSpecular>("Node_DiffuseSpecular");
  managerId_ = matManager_.addMaterial(mat);

  inDiffuseTex_       = addInputPin(pinKeyType(PinType::kColorTexture2D), "Diffuse");
  inSpecularGlossTex_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Specular/Gloss");
  inNormalTex_        = addInputPin(pinKeyType(PinType::kColorTexture2D), "Normal Map");

  outMaterial_ = addOutputPin(pinKeyType(PinType::kMaterial), "Material");

  MaterialLink link{.type = material::Type::kDiffuseSpecular, .managerId = managerId_};
  outMaterial_->setData<MaterialLink>(link);

  onStateChanged = [this]() {
    if (status_ != NodeStatus::kStale) return;
    auto m = matManager_.getMaterial(material::Type::kDiffuseSpecular, managerId_);
    if (!m) return;
    auto ds = std::static_pointer_cast<material::DiffuseSpecular>(m);

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

    strip_loading_tex(inDiffuseTex_,       [&]() { ds->setDiffuseTexture(nullptr); });
    strip_loading_tex(inSpecularGlossTex_, [&]() { ds->setSpecularGlossinessTexture(nullptr); });
    strip_loading_tex(inNormalTex_,        [&]() { ds->setNormalTexture(nullptr); });

    if (changed) ds->setDirty(true);
  };
}

DiffuseSpecularNode::~DiffuseSpecularNode() {
  matManager_.removeMaterial(material::Type::kDiffuseSpecular, managerId_);
}

void DiffuseSpecularNode::execute() {
  auto mat = matManager_.getMaterial(material::Type::kDiffuseSpecular, managerId_);
  if (!mat) {
    setStatus(NodeStatus::kError);
    return;
  }

  auto ds = std::static_pointer_cast<material::DiffuseSpecular>(mat);
  ds->setAlphaMode(alphaMode);
  ds->params.diffuseFactor    = diffuseFactor;
  ds->params.specularFactor   = specularFactor;
  ds->params.glossinessFactor = glossinessFactor;

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

  resolve_tex(inDiffuseTex_,       [&](const auto& t) { ds->setDiffuseTexture(t); });
  resolve_tex(inSpecularGlossTex_, [&](const auto& t) { ds->setSpecularGlossinessTexture(t); });
  resolve_tex(inNormalTex_,        [&](const auto& t) { ds->setNormalTexture(t); });

  ds->setDirty(true);

  if (waiting) {
    setStatus(NodeStatus::kStale);
    propagateStale();
  } else {
    setStatus(NodeStatus::kReady);
  }
}

};  // namespace vkit::workflow::node::mat
