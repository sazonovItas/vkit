#pragma once

#include <cstdint>

namespace vkit::core::events {

struct FractalPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  uint32_t fractalType{0};
  int32_t maxIterations{256};
  float centerX{-0.5F};
  float centerY{0.0F};
  float zoom{1.0F};
  float juliaRe{-0.7F};
  float juliaIm{0.27F};
};

}  // namespace vkit::core::events
