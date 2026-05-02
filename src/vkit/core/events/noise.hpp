#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/texture.hpp"

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

struct NoiseJobRequest {
  std::uint64_t requestId{0};
  NoisePushConstants params;
};

struct NoiseJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using NoiseJobBus = message_bus::MessageBus<NoiseJobRequest,
                                            message_bus::DispatchPolicy::kBoth>;
using NoiseResultBus =
    message_bus::MessageBus<NoiseJobResult, message_bus::DispatchPolicy::kBoth>;

};  // namespace vkit::core::events
