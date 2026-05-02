#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/loader.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::core::events {

struct TextureLoadRequest {
  std::filesystem::path path;
  texture::LoadOptions options{};
  std::uint64_t requestId{0};
};

struct TextureReadyEvent {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> texture;
  std::string error;
};

using TextureLoadBus =
    message_bus::MessageBus<TextureLoadRequest,
                            message_bus::DispatchPolicy::kBoth>;
using TextureReadyBus =
    message_bus::MessageBus<TextureReadyEvent,
                            message_bus::DispatchPolicy::kBoth>;

};  // namespace vkit::core::events
