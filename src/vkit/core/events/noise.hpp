#pragma once

#include <cstdint>

namespace vkit::core::events {

struct NoisePushConstants {
  uint32_t width{512};
  uint32_t height{512};
  uint32_t noiseType{0};
  uint32_t worleyMode{0};
  float scale{4.0F};
  float offsetX{0.0F};
  float offsetY{0.0F};
  float seed{0.0F};
  int32_t octaves{6};
  float persistence{0.5F};
  float lacunarity{2.0F};
  float worleyJitter{1.0F};
};

}  // namespace vkit::core::events
