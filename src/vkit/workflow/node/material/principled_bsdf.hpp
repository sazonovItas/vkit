#pragma once

#include <glm/glm.hpp>
#include <string_view>

#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::mat {

class PrincipledBSDFNode : public WorkflowNode {
 public:
  PrincipledBSDFNode(std::string_view name,
                     material::MaterialManager& matManager,
                     texture::TextureManager& texManager);
  ~PrincipledBSDFNode() override;

  void execute() override;

  material::AlphaMode alphaMode{material::AlphaMode::kOpaque};

  glm::vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
  glm::vec3 emissiveFactor{0.0F, 0.0F, 0.0F};
  float metallicFactor{0.0F};
  float roughnessFactor{0.5F};
  float occlusionStrength{1.0F};
  float ior{1.5F};

  glm::vec3 specularColorFactor{1.0F, 1.0F, 1.0F};
  float specularFactor{1.0F};

  float transmissionFactor{0.0F};
  float thicknessFactor{0.0F};
  float attenuationDistance{0.0F};
  glm::vec3 attenuationColor{1.0F, 1.0F, 1.0F};

  glm::vec3 sheenColorFactor{0.0F, 0.0F, 0.0F};
  float sheenRoughnessFactor{0.0F};

  float clearcoatFactor{0.0F};
  float clearcoatRoughnessFactor{0.0F};

  float anisotropyStrength{0.0F};
  glm::vec2 anisotropyRotation{1.0F, 0.0F};

  float iridescenceFactor{0.0F};
  float iridescenceIor{1.5F};
  float iridescenceThicknessMin{100.0F};
  float iridescenceThicknessMax{400.0F};

 private:
  material::MaterialManager& matManager_;
  texture::TextureManager& texManager_;
  std::uint32_t managerId_{vkit::kStorageItemInvalidId};

  graph::Pin* inBaseColorTex_{nullptr};
  graph::Pin* inNormalTex_{nullptr};
  graph::Pin* inMetallicRoughnessTex_{nullptr};
  graph::Pin* inEmissiveTex_{nullptr};
  graph::Pin* inOcclusionTex_{nullptr};

  graph::Pin* inSpecularTex_{nullptr};
  graph::Pin* inSpecularColorTex_{nullptr};

  graph::Pin* inTransmissionThicknessTex_{nullptr};

  graph::Pin* inClearcoatTex_{nullptr};
  graph::Pin* inClearcoatNormalTex_{nullptr};

  graph::Pin* inSheenColorTex_{nullptr};
  graph::Pin* inSheenRoughnessTex_{nullptr};

  graph::Pin* inAnisotropyTex_{nullptr};

  graph::Pin* inIridescenceTex_{nullptr};
  graph::Pin* inIridescenceThicknessTex_{nullptr};

  graph::Pin* outMaterial_{nullptr};
};

};  // namespace vkit::workflow::node::mat
