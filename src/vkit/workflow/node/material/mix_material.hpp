#pragma once

#include <string_view>

#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::mat {

class MixMaterialNode : public WorkflowNode {
 public:
  MixMaterialNode(std::string_view name,
                  material::MaterialManager& matManager,
                  texture::TextureManager& texManager);
  ~MixMaterialNode() override;

  void execute() override;

  float factor{0.5F};
  float threshold{0.5F};
  float edge{1.0F};
  float alphaCutoff{0.01F};
  material::AlphaMode alphaMode{material::AlphaMode::kOpaque};

 private:
  material::MaterialManager& matManager_;
  texture::TextureManager& texManager_;
  std::uint32_t managerId_{vkit::kStorageItemInvalidId};

  graph::Pin* inMatA_{nullptr};
  graph::Pin* inMatB_{nullptr};
  graph::Pin* inFactor_{nullptr};
  graph::Pin* inOpacity_{nullptr};
  graph::Pin* outMaterial_{nullptr};
};

}  // namespace vkit::workflow::node::mat
