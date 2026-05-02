#pragma once

#include "vkit/core/events/compute.hpp"
#include "vkit/core/events/texture.hpp"

namespace vkit::workflow {

struct ExecutionContext {
  core::events::TextureLoadBus texLoadBus;
  core::events::TextureReadyBus texReadyBus;
  core::events::ComputeJobBus computeJobBus;
  core::events::ComputeResultBus computeResultBus;

  void update() {
    texLoadBus.update();
    computeJobBus.update();
    texReadyBus.update();
    computeResultBus.update();
  }
};

};  // namespace vkit::workflow
