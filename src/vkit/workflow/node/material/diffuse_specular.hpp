#pragma once

#include <glm/glm.hpp>
#include <string_view>

#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::mat {

class DiffuseSpecularNode : public WorkflowNode {
 public:
  DiffuseSpecularNode(std::string_view name,
                      material::MaterialManager& matManager,
                      texture::TextureManager& texManager);
  ~DiffuseSpecularNode() override;

  void execute() override;

  material::AlphaMode alphaMode{material::AlphaMode::kOpaque};
  glm::vec4 diffuseFactor{1.0F, 1.0F, 1.0F, 1.0F};
  glm::vec3 specularFactor{1.0F, 1.0F, 1.0F};
  float glossinessFactor{1.0F};

 private:
  material::MaterialManager& matManager_;
  texture::TextureManager& texManager_;
  std::uint32_t managerId_{vkit::kStorageItemInvalidId};

  graph::Pin* inDiffuseTex_{nullptr};
  graph::Pin* inSpecularGlossTex_{nullptr};
  graph::Pin* inNormalTex_{nullptr};
  graph::Pin* outMaterial_{nullptr};
};

};  // namespace vkit::workflow::node::mat
