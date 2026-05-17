#pragma once

#include <glm/glm.hpp>
#include <string_view>

#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::mat {

class DiffuseNode : public WorkflowNode {
 public:
  DiffuseNode(std::string_view name, material::MaterialManager& matManager,
              texture::TextureManager& texManager);
  ~DiffuseNode() override;

  void execute() override;

  material::AlphaMode alphaMode{material::AlphaMode::kOpaque};
  glm::vec4 diffuseFactor{1.0F, 1.0F, 1.0F, 1.0F};

 private:
  material::MaterialManager& matManager_;
  texture::TextureManager& texManager_;
  std::uint32_t managerId_{vkit::kStorageItemInvalidId};

  graph::Pin* inDiffuseTex_{nullptr};
  graph::Pin* inNormalTex_{nullptr};
  graph::Pin* outMaterial_{nullptr};
};

};  // namespace vkit::workflow::node::mat
