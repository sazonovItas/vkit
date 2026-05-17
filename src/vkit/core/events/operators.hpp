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

struct ChannelAdjustPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float gainR{1.0F}; float gainG{1.0F}; float gainB{1.0F}; float gainA{1.0F};
  float biasR{0.0F}; float biasG{0.0F}; float biasB{0.0F}; float biasA{0.0F};
  uint32_t invertR{0}; uint32_t invertG{0}; uint32_t invertB{0}; uint32_t invertA{0};
};

// Channel source: 0=R 1=G 2=B 3=A 4=const0 5=const1
struct ChannelRemapPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  uint32_t outR{0};
  uint32_t outG{1};
  uint32_t outB{2};
  uint32_t outA{3};
};

}  // namespace vkit::core::events
