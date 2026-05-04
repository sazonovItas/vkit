#pragma once

#include "vkit/core/events/compute_output.hpp"
#include "vkit/core/events/texture.hpp"

namespace vkit::workflow {

struct ExecutionContext {
  core::events::TextureLoadBus texLoadBus;
  core::events::TextureReadyBus texReadyBus;
  core::events::ComputeOutputBus computeOutputBus;
  core::events::ComputeOutputResultBus computeOutputResultBus;

  void update() {
    texLoadBus.update();
    texReadyBus.update();
    computeOutputBus.update();
    computeOutputResultBus.update();
  }
};

}  // namespace vkit::workflow
