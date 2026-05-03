#pragma once

#include <memory>

#include "vkit/graphics/device.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::compute::util {

[[nodiscard]] auto makeOutputTexture(const graphics::GfxDevice& device,
                                     uint32_t w, uint32_t h, vk::Format fmt,
                                     vk::Sampler sampler, std::string_view name)
    -> std::shared_ptr<texture::Texture>;

};  // namespace vkit::compute::util
