#pragma once

#include <cstdint>

namespace vkit::core::events {

struct PatternPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  uint32_t patternType{0};

  float scale{5.0F};
  float thickness{0.1F};
  float smoothness{0.05F};

  float param1{0.5F};
  float param2{0.0F};
};

}  // namespace vkit::core::events
