#pragma once

#include "vkit/core/events/noise.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/core/events/texture.hpp"

namespace vkit::workflow {

struct ExecutionContext {
  core::events::TextureLoadBus texLoadBus;
  core::events::TextureReadyBus texReadyBus;
  core::events::NoiseJobBus noiseJobBus;
  core::events::NoiseResultBus noiseResultBus;
  core::events::SobelJobBus sobelJobBus;
  core::events::SobelResultBus sobelResultBus;
  core::events::HeightMapJobBus heightMapJobBus;
  core::events::HeightMapResultBus heightMapResultBus;
  core::events::TintJobBus tintJobBus;
  core::events::TintResultBus tintResultBus;

  void update() {
    texLoadBus.update();
    noiseJobBus.update();
    texReadyBus.update();
    noiseResultBus.update();
    sobelJobBus.update();
    sobelResultBus.update();
    heightMapJobBus.update();
    heightMapResultBus.update();
    tintJobBus.update();
    tintResultBus.update();
  }
};

};  // namespace vkit::workflow
