#include "vkit/texture/texture.hpp"

namespace vkit::texture {

Texture::Texture(std::string_view name,
                 const std::shared_ptr<vkit::graphics::Texture>& texture)
    : Item<Texture>{name}, texture_{texture} {}

auto Texture::getStorageId() const -> std::optional<std::uint32_t> {
  return storageId_;
}

auto Texture::getBindlessId() const -> std::optional<std::uint32_t> {
  return bindlessId_;
}

auto Texture::getImguiId() const -> std::optional<ImTextureID> {
  return imguiId_;
}

auto Texture::getGraphicsTexture() const
    -> std::shared_ptr<vkit::graphics::Texture> {
  return texture_;
}

void Texture::setStorageId(std::optional<std::uint32_t> id) { storageId_ = id; }

void Texture::setBindlessId(std::optional<std::uint32_t> id) {
  bindlessId_ = id;
}

void Texture::setImguiId(std::optional<ImTextureID> id) { imguiId_ = id; }

}  // namespace vkit::texture
