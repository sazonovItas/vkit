#pragma once

#include "vkit/core/events/noise.hpp"
#include "vkit/core/events/texture.hpp"

namespace vkit::workflow {

struct ExecutionContext {
  core::events::TextureLoadBus texLoadBus;
  core::events::TextureReadyBus texReadyBus;
  core::events::NoiseJobBus noiseJobBus;
  core::events::NoiseResultBus noiseResultBus;

  void update() {
    texLoadBus.update();
    noiseJobBus.update();
    texReadyBus.update();
    noiseResultBus.update();
  }
};

};  // namespace vkit::workflow
