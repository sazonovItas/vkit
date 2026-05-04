#pragma once

#include <string>
#include <vector>

#include "vkit/core/events/noise.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/node/material/principled_bsdf.hpp"
#include "vkit/workflow/node/material/slot_output.hpp"
#include "vkit/workflow/node/operators/heightmap.hpp"
#include "vkit/workflow/node/operators/mix.hpp"
#include "vkit/workflow/node/operators/normalmap.hpp"
#include "vkit/workflow/node/operators/sobel.hpp"
#include "vkit/workflow/node/operators/tint.hpp"
#include "vkit/workflow/node/procedural/noise_generator.hpp"
#include "vkit/workflow/node/texture_load.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::controller {

class WorkflowController {
 public:
  WorkflowController() = default;
  ~WorkflowController() = default;

  auto setWorkflow(workflow::Workflow* workflow) -> WorkflowController&;

  auto setTextureManager(texture::TextureManager* textureManager)
      -> WorkflowController&;

  auto setTextureLoadBus(core::events::TextureLoadBus* bus)
      -> WorkflowController&;
  auto setTextureReadyBus(core::events::TextureReadyBus* bus)
      -> WorkflowController&;

  auto setNoiseJobBus(core::events::NoiseJobBus* bus) -> WorkflowController&;
  auto setNoiseResultBus(core::events::NoiseResultBus* bus)
      -> WorkflowController&;

  auto setSobelJobBus(core::events::SobelJobBus* bus) -> WorkflowController&;
  auto setSobelResultBus(core::events::SobelResultBus* bus)
      -> WorkflowController&;

  auto setHeightMapJobBus(core::events::HeightMapJobBus* bus)
      -> WorkflowController&;
  auto setHeightMapResultBus(core::events::HeightMapResultBus* bus)
      -> WorkflowController&;

  auto setNormalMapJobBus(core::events::NormalMapJobBus* bus)
      -> WorkflowController&;
  auto setNormalMapResultBus(core::events::NormalMapResultBus* bus)
      -> WorkflowController&;

  auto setTintJobBus(core::events::TintJobBus* bus) -> WorkflowController&;
  auto setTintResultBus(core::events::TintResultBus* bus)
      -> WorkflowController&;

  auto setMixJobBus(core::events::MixJobBus* bus) -> WorkflowController&;
  auto setMixResultBus(core::events::MixResultBus* bus) -> WorkflowController&;

  auto setMaterialManager(material::MaterialManager* materialManager)
      -> WorkflowController&;

  auto createTextureLoadNode(const std::string& name = "Texture Load")
      -> workflow::node::TextureLoadNode*;
  auto createNoiseGeneratorNode(const std::string& name = "Noise Generator")
      -> workflow::node::proc::NoiseGeneratorNode*;
  auto createSobelNode(const std::string& name = "Sobel Edge")
      -> workflow::node::op::SobelNode*;
  auto createHeightMapNode(const std::string& name = "Height Map")
      -> workflow::node::op::HeightMapNode*;
  auto createNormalMapNode(const std::string& name = "Normal Map")
      -> workflow::node::op::NormalMapNode*;
  auto createTintNode(const std::string& name = "Color Tint")
      -> workflow::node::op::TintNode*;
  auto createMixNode(const std::string& name = "Mix")
      -> workflow::node::op::MixNode*;

  auto createPrincipledBSDFNode(const std::string& name = "Principled BSDF")
      -> workflow::node::mat::PrincipledBSDFNode*;
  auto createSlotOutputNode(const std::string& name = "Material Slot")
      -> workflow::node::mat::SlotOutputNode*;

  void deleteNodes(const std::vector<int>& nodeIds);

  [[nodiscard]] auto canConnectPins(int pinIdA, int pinIdB) const -> bool;
  void connectPins(int sourcePinId, int targetPinId);

  void disconnectLink(int linkId);

  [[nodiscard]] auto getWorkflow() const -> workflow::Workflow* {
    return workflow_;
  }

 private:
  workflow::Workflow* workflow_{nullptr};
  texture::TextureManager* textureManager_{nullptr};
  material::MaterialManager* materialManager_{nullptr};

  core::events::TextureLoadBus* textureLoadBus_{nullptr};
  core::events::TextureReadyBus* textureReadyBus_{nullptr};

  core::events::NoiseJobBus* noiseJobBus_{nullptr};
  core::events::NoiseResultBus* noiseResultBus_{nullptr};

  core::events::SobelJobBus* sobelJobBus_{nullptr};
  core::events::SobelResultBus* sobelResultBus_{nullptr};

  core::events::HeightMapJobBus* heightMapJobBus_{nullptr};
  core::events::HeightMapResultBus* heightMapResultBus_{nullptr};

  core::events::NormalMapJobBus* normalMapJobBus_{nullptr};
  core::events::NormalMapResultBus* normalMapResultBus_{nullptr};

  core::events::TintJobBus* tintJobBus_{nullptr};
  core::events::TintResultBus* tintResultBus_{nullptr};

  core::events::MixJobBus* mixJobBus_{nullptr};
  core::events::MixResultBus* mixResultBus_{nullptr};
};

};  // namespace vkit::controller
