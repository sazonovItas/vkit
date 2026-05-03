#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::core::events {

struct SobelPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float intensity{1.0F};
  float threshold{0.1F};
};

struct SobelJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> inputTexture;
  SobelPushConstants params;
};

struct SobelJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using SobelJobBus = message_bus::MessageBus<SobelJobRequest,
                                            message_bus::DispatchPolicy::kBoth>;
using SobelResultBus =
    message_bus::MessageBus<SobelJobResult, message_bus::DispatchPolicy::kBoth>;

struct HeightMapPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float contrast{1.0F};
  float brightness{0.0F};
  uint32_t invert{0};
};

struct HeightMapJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> inputTexture;
  HeightMapPushConstants params;
};

struct HeightMapJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using HeightMapJobBus =
    message_bus::MessageBus<HeightMapJobRequest,
                            message_bus::DispatchPolicy::kBoth>;
using HeightMapResultBus =
    message_bus::MessageBus<HeightMapJobResult,
                            message_bus::DispatchPolicy::kBoth>;

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

struct TintJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> inputTexture;
  TintPushConstants params;
};

struct TintJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using TintJobBus =
    message_bus::MessageBus<TintJobRequest, message_bus::DispatchPolicy::kBoth>;
using TintResultBus =
    message_bus::MessageBus<TintJobResult, message_bus::DispatchPolicy::kBoth>;

struct NormalMapPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float strength{1.0F};
  uint32_t invertX{0};
  uint32_t invertY{0};
};

struct NormalMapJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> inputTexture;
  NormalMapPushConstants params;
};

struct NormalMapJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using NormalMapJobBus =
    message_bus::MessageBus<NormalMapJobRequest,
                            message_bus::DispatchPolicy::kBoth>;
using NormalMapResultBus =
    message_bus::MessageBus<NormalMapJobResult,
                            message_bus::DispatchPolicy::kBoth>;

struct MixPushConstants {
  uint32_t width{512};
  uint32_t height{512};
  float factor{0.5F};
  uint32_t useFacTex{0};
  uint32_t mode{0};
};

struct MixJobRequest {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> inputA;
  std::shared_ptr<texture::Texture> inputB;
  std::shared_ptr<texture::Texture> inputFac;
  MixPushConstants params;
};

struct MixJobResult {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> imageF32;
  std::shared_ptr<texture::Texture> imageUnorm;
  std::string error;
};

using MixJobBus =
    message_bus::MessageBus<MixJobRequest, message_bus::DispatchPolicy::kBoth>;
using MixResultBus =
    message_bus::MessageBus<MixJobResult, message_bus::DispatchPolicy::kBoth>;

};  // namespace vkit::core::events
