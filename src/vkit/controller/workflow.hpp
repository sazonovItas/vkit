#pragma once

#include <string>
#include <vector>

#include "vkit/core/events/noise.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/node/heightmap.hpp"
#include "vkit/workflow/node/noise_generator.hpp"
#include "vkit/workflow/node/sobel.hpp"
#include "vkit/workflow/node/texture_load.hpp"
#include "vkit/workflow/node/tint.hpp"
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

  auto setTintJobBus(core::events::TintJobBus* bus) -> WorkflowController&;
  auto setTintResultBus(core::events::TintResultBus* bus)
      -> WorkflowController&;

  auto createTextureLoadNode(const std::string& name = "Texture Load")
      -> workflow::node::TextureLoadNode*;
  auto createNoiseGeneratorNode(const std::string& name = "Noise Generator")
      -> workflow::node::NoiseGeneratorNode*;
  auto createSobelNode(const std::string& name = "Sobel Edge")
      -> workflow::node::SobelNode*;
  auto createHeightMapNode(const std::string& name = "Height Map")
      -> workflow::node::HeightMapNode*;
  auto createTintNode(const std::string& name = "Color Tint")
      -> workflow::node::TintNode*;

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

  core::events::TextureLoadBus* textureLoadBus_{nullptr};
  core::events::TextureReadyBus* textureReadyBus_{nullptr};

  core::events::NoiseJobBus* noiseJobBus_{nullptr};
  core::events::NoiseResultBus* noiseResultBus_{nullptr};

  core::events::SobelJobBus* sobelJobBus_{nullptr};
  core::events::SobelResultBus* sobelResultBus_{nullptr};

  core::events::HeightMapJobBus* heightMapJobBus_{nullptr};
  core::events::HeightMapResultBus* heightMapResultBus_{nullptr};

  core::events::TintJobBus* tintJobBus_{nullptr};
  core::events::TintResultBus* tintResultBus_{nullptr};
};

};  // namespace vkit::controller
