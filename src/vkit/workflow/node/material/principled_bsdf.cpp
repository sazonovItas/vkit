#include "vkit/workflow/node/material/principled_bsdf.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/workflow/node/material/material_link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::mat {

PrincipledBSDFNode::PrincipledBSDFNode(std::string_view name,
                                       material::MaterialManager& matManager,
                                       texture::TextureManager& texManager)
    : WorkflowNode(name), matManager_(matManager), texManager_(texManager) {
  auto mat = std::make_shared<material::PrincipledBSDF>("Node_PrincipledBSDF");
  managerId_ = matManager_.addMaterial(mat);

  inBaseColorTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Base Color");
  inNormalTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Normal Map");
  inMetallicRoughnessTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Metallic/Roughness");
  inEmissiveTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Emissive");
  inOcclusionTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Ambient Occlusion");

  inSpecularTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Specular");
  inSpecularColorTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Specular Color");

  inTransmissionThicknessTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Thickness");

  inClearcoatTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Clearcoat");
  inClearcoatNormalTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Clearcoat Normal");

  inSheenColorTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Sheen Color");
  inSheenRoughnessTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Sheen Roughness");

  inAnisotropyTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Anisotropy");

  inIridescenceTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Iridescence");
  inIridescenceThicknessTex_ =
      addInputPin(pinKeyType(PinType::kColorTexture2D), "Irid. Thickness");

  outMaterial_ = addOutputPin(pinKeyType(PinType::kMaterial), "Material");

  MaterialLink link{
      .type = material::Type::kPrincipledBSDF,
      .managerId = managerId_,
  };
  outMaterial_->setData<MaterialLink>(link);

  onStateChanged = [this]() {
    if (status_ == NodeStatus::kStale) {
      auto m =
          matManager_.getMaterial(material::Type::kPrincipledBSDF, managerId_);
      if (!m) return;
      auto bsdf = std::static_pointer_cast<material::PrincipledBSDF>(m);

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

      strip_loading_tex(inBaseColorTex_,
                        [&]() { bsdf->setBaseColorTexture(nullptr); });
      strip_loading_tex(inNormalTex_,
                        [&]() { bsdf->setNormalTexture(nullptr); });
      strip_loading_tex(inMetallicRoughnessTex_,
                        [&]() { bsdf->setMetallicRoughnessTexture(nullptr); });
      strip_loading_tex(inEmissiveTex_,
                        [&]() { bsdf->setEmissiveTexture(nullptr); });
      strip_loading_tex(inOcclusionTex_,
                        [&]() { bsdf->setOcclusionTexture(nullptr); });
      strip_loading_tex(inSpecularTex_,
                        [&]() { bsdf->setSpecularTexture(nullptr); });
      strip_loading_tex(inSpecularColorTex_,
                        [&]() { bsdf->setSpecularColorTexture(nullptr); });
      strip_loading_tex(inTransmissionThicknessTex_,
                        [&]() { bsdf->setThicknessTexture(nullptr); });
      strip_loading_tex(inClearcoatTex_,
                        [&]() { bsdf->setClearcoatTexture(nullptr); });
      strip_loading_tex(inClearcoatNormalTex_,
                        [&]() { bsdf->setClearcoatNormalTexture(nullptr); });
      strip_loading_tex(inSheenColorTex_,
                        [&]() { bsdf->setSheenColorTexture(nullptr); });
      strip_loading_tex(inSheenRoughnessTex_,
                        [&]() { bsdf->setSheenRoughnessTexture(nullptr); });
      strip_loading_tex(inAnisotropyTex_,
                        [&]() { bsdf->setAnisotropyTexture(nullptr); });
      strip_loading_tex(inIridescenceTex_,
                        [&]() { bsdf->setIridescenceTexture(nullptr); });
      strip_loading_tex(inIridescenceThicknessTex_, [&]() {
        bsdf->setIridescenceThicknessTexture(nullptr);
      });

      if (changed) bsdf->setDirty(true);
    }
  };
}

PrincipledBSDFNode::~PrincipledBSDFNode() {
  matManager_.removeMaterial(material::Type::kPrincipledBSDF, managerId_);
}

void PrincipledBSDFNode::execute() {
  auto mat =
      matManager_.getMaterial(material::Type::kPrincipledBSDF, managerId_);
  if (!mat) {
    setStatus(NodeStatus::kError);
    return;
  }

  auto bsdf = std::static_pointer_cast<material::PrincipledBSDF>(mat);

  // Apply properties
  bsdf->setAlphaMode(alphaMode);

  bsdf->params.baseColorFactor = baseColorFactor;
  bsdf->params.emissiveFactor = emissiveFactor;
  bsdf->params.metallicFactor = metallicFactor;
  bsdf->params.roughnessFactor = roughnessFactor;
  bsdf->params.occlusionStrength = occlusionStrength;
  bsdf->params.ior = ior;

  bsdf->params.specularColorFactor = specularColorFactor;
  bsdf->params.specularFactor = specularFactor;

  bsdf->params.transmissionFactor = transmissionFactor;
  bsdf->params.thicknessFactor = thicknessFactor;
  bsdf->params.attenuationDistance = attenuationDistance;
  bsdf->params.attenuationColor = attenuationColor;

  bsdf->params.sheenColorFactor = sheenColorFactor;
  bsdf->params.sheenRoughnessFactor = sheenRoughnessFactor;

  bsdf->params.clearcoatFactor = clearcoatFactor;
  bsdf->params.clearcoatRoughnessFactor = clearcoatRoughnessFactor;

  bsdf->params.anisotropyStrength = anisotropyStrength;
  bsdf->params.anisotropyRotation = anisotropyRotation;

  bsdf->params.iridescenceFactor = iridescenceFactor;
  bsdf->params.iridescenceIor = iridescenceIor;
  bsdf->params.iridescenceThicknessMin = iridescenceThicknessMin;
  bsdf->params.iridescenceThicknessMax = iridescenceThicknessMax;

  bool waiting_for_inputs = false;

  auto resolve_tex = [&](graph::Pin* pin, auto setter) {
    if (pin->hasData()) {
      setter(texManager_.get(*pin->getData<std::uint32_t>()));
      return;
    }
    auto links = pin->getLinks();
    if (links.empty()) {
      setter(nullptr);
      return;
    }
    auto* src_pin = links.front()->getSrc();
    auto* src_node =
        static_cast<workflow::WorkflowNode*>(src_pin->getOwnerNode());

    if (src_node->status() != NodeStatus::kReady) {
      waiting_for_inputs = true;
      setter(nullptr);
      return;
    }
    if (const auto* id = src_pin->getData<std::uint32_t>()) {
      setter(texManager_.get(*id));
    } else {
      setter(nullptr);
    }
  };

  resolve_tex(inBaseColorTex_,
              [&](const auto& t) { bsdf->setBaseColorTexture(t); });
  resolve_tex(inNormalTex_, [&](const auto& t) { bsdf->setNormalTexture(t); });
  resolve_tex(inMetallicRoughnessTex_,
              [&](const auto& t) { bsdf->setMetallicRoughnessTexture(t); });
  resolve_tex(inEmissiveTex_,
              [&](const auto& t) { bsdf->setEmissiveTexture(t); });
  resolve_tex(inOcclusionTex_,
              [&](const auto& t) { bsdf->setOcclusionTexture(t); });
  resolve_tex(inSpecularTex_,
              [&](const auto& t) { bsdf->setSpecularTexture(t); });
  resolve_tex(inSpecularColorTex_,
              [&](const auto& t) { bsdf->setSpecularColorTexture(t); });
  resolve_tex(inTransmissionThicknessTex_,
              [&](const auto& t) { bsdf->setThicknessTexture(t); });
  resolve_tex(inClearcoatTex_,
              [&](const auto& t) { bsdf->setClearcoatTexture(t); });
  resolve_tex(inClearcoatNormalTex_,
              [&](const auto& t) { bsdf->setClearcoatNormalTexture(t); });
  resolve_tex(inSheenColorTex_,
              [&](const auto& t) { bsdf->setSheenColorTexture(t); });
  resolve_tex(inSheenRoughnessTex_,
              [&](const auto& t) { bsdf->setSheenRoughnessTexture(t); });
  resolve_tex(inAnisotropyTex_,
              [&](const auto& t) { bsdf->setAnisotropyTexture(t); });
  resolve_tex(inIridescenceTex_,
              [&](const auto& t) { bsdf->setIridescenceTexture(t); });
  resolve_tex(inIridescenceThicknessTex_,
              [&](const auto& t) { bsdf->setIridescenceThicknessTexture(t); });

  bsdf->params.featureMask = 0;
  if (glm::length(emissiveFactor) > 0.001F || bsdf->getEmissiveTexture())
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kEmissive);

  if (transmissionFactor > 0.001F)
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kTransmission);

  if (clearcoatFactor > 0.001F || bsdf->getClearcoatTexture())
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kClearcoat);

  if (glm::length(sheenColorFactor) > 0.001F || bsdf->getSheenColorTexture())
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kSheen);

  if (anisotropyStrength > 0.001F || bsdf->getAnisotropyTexture())
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kAnisotropy);

  if (iridescenceFactor > 0.001F || bsdf->getIridescenceTexture())
    bsdf->enableFeature(material::PrincipledBSDF::Feature::kIridescence);

  bsdf->setDirty(true);

  if (waiting_for_inputs) {
    setStatus(NodeStatus::kStale);
    propagateStale();
  } else {
    setStatus(NodeStatus::kReady);
  }
}

};  // namespace vkit::workflow::node::mat
