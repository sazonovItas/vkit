#pragma once

#include <cstdint>

namespace vkit::core::events {

struct SobelPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float intensity{1.0F};
  float threshold{0.1F};
};

struct HeightMapPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float contrast{1.0F};
  float brightness{0.0F};
  uint32_t invert{0};
};

struct TintPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float rCoef{1.0F};
  float gCoef{1.0F};
  float bCoef{1.0F};
  float aCoef{1.0F};
  float factor{1.0F};
  uint32_t mode{1};
};

struct NormalMapPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float strength{1.0F};
  uint32_t invertX{0};
  uint32_t invertY{0};
};

struct MixPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float factor{0.5F};
  uint32_t useFacTex{0};
  uint32_t mode{0};
};

}  // namespace vkit::core::events
