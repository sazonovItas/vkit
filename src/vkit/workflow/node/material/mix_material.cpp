#include "vkit/workflow/node/material/mix_material.hpp"

#include <glm/glm.hpp>

#include "vkit/graph/link.hpp"
#include "vkit/material/mix_material.hpp"
#include "vkit/material/principled_bsdf.hpp"
#include "vkit/workflow/node/material/material_link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::mat {

MixMaterialNode::MixMaterialNode(std::string_view name,
                                 material::MaterialManager& matManager,
                                 texture::TextureManager& texManager)
    : WorkflowNode(name), matManager_(matManager), texManager_(texManager) {
  auto mat = std::make_shared<material::MixMaterial>("Node_MixMaterial");
  managerId_ = matManager_.addMaterial(mat);

  inMatA_      = addInputPin(pinKeyType(PinType::kMaterial), "Material A");
  inMatB_      = addInputPin(pinKeyType(PinType::kMaterial), "Material B");
  inFactor_    = addInputPin(pinKeyType(PinType::kColorTexture2D), "Factor Map");
  inOpacity_   = addInputPin(pinKeyType(PinType::kFloatTexture2D), "Opacity Map");
  outMaterial_ = addOutputPin(pinKeyType(PinType::kMaterial), "Material");

  MaterialLink link{
      .type = material::Type::kMix,
      .managerId = managerId_,
  };
  outMaterial_->setData<MaterialLink>(link);
}

MixMaterialNode::~MixMaterialNode() {
  matManager_.removeMaterial(material::Type::kMix, managerId_);
}

void MixMaterialNode::execute() {
  auto mat_base = matManager_.getMaterial(material::Type::kMix, managerId_);
  if (!mat_base) {
    setStatus(NodeStatus::kError);
    return;
  }
  auto mix_mat = std::static_pointer_cast<material::MixMaterial>(mat_base);

  mix_mat->setAlphaMode(alphaMode);

  bool waiting = false;

  // Resolve material A — must be PrincipledBSDF.
  auto resolve_mat = [&](graph::Pin* pin) -> std::uint32_t {
    const MaterialLink* link = nullptr;
    if (pin->hasData()) {
      link = pin->getData<MaterialLink>();
    } else {
      const auto& links = pin->getLinks();
      if (!links.empty()) {
        auto* src_pin  = links.front()->getSrc();
        auto* src_node = static_cast<WorkflowNode*>(src_pin->getOwnerNode());
        if (src_node->status() != NodeStatus::kReady) {
          waiting = true;
          return 0;
        }
        link = src_pin->getData<MaterialLink>();
      }
    }
    if (link && link->type == material::Type::kPrincipledBSDF)
      return link->managerId;
    return 0;
  };

  std::uint32_t idx_a = resolve_mat(inMatA_);
  std::uint32_t idx_b = resolve_mat(inMatB_);

  // Resolve optional factor texture.
  auto resolve_tex = [&](graph::Pin* pin) -> std::shared_ptr<texture::Texture> {
    if (pin->hasData()) {
      if (const auto* id = pin->getData<std::uint32_t>())
        return texManager_.get(*id);
      return nullptr;
    }
    const auto& links = pin->getLinks();
    if (links.empty()) return nullptr;

    auto* src_pin  = links.front()->getSrc();
    auto* src_node = static_cast<WorkflowNode*>(src_pin->getOwnerNode());
    if (src_node->status() != NodeStatus::kReady) {
      waiting = true;
      return nullptr;
    }
    if (const auto* id = src_pin->getData<std::uint32_t>())
      return texManager_.get(*id);
    return nullptr;
  };

  auto factor_tex  = resolve_tex(inFactor_);
  auto opacity_tex = resolve_tex(inOpacity_);

  auto has_transmission = [&](std::uint32_t idx) -> bool {
    auto base = matManager_.getMaterial(material::Type::kPrincipledBSDF, idx);
    if (!base) return false;
    return static_cast<material::PrincipledBSDF*>(base.get())
        ->hasFeature(material::PrincipledBSDF::Feature::kTransmission);
  };
  mix_mat->setHasTransmission(has_transmission(idx_a) || has_transmission(idx_b));

  mix_mat->params.materialIndexA = idx_a;
  mix_mat->params.materialIndexB = idx_b;
  mix_mat->params.factor      = glm::clamp(factor, 0.0F, 1.0F);
  mix_mat->params.threshold   = glm::clamp(threshold, 0.0F, 1.0F);
  mix_mat->params.edge        = glm::clamp(edge, 0.0F, 2.0F);
  mix_mat->params.alphaCutoff = glm::clamp(alphaCutoff, 0.0F, 1.0F);
  mix_mat->setFactorTexture(factor_tex);
  mix_mat->setOpacityTexture(opacity_tex);
  mix_mat->setDirty(true);

  if (waiting) {
    setStatus(NodeStatus::kStale);
    propagateStale();
  } else {
    setStatus(NodeStatus::kReady);
  }
}

}  // namespace vkit::workflow::node::mat
