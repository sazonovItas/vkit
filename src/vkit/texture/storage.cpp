#include "vkit/texture/storage.hpp"

#include "vkit/graphics/bindless_texture_manager.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/item/storage_item.hpp"

namespace vkit::texture {

TextureStorage::~TextureStorage() {
  const auto lock = std::scoped_lock{this->mutex_};

  for (auto& texture : this->items_) {
    if (texture) {
      if ((bindlessManager_ != nullptr) &&
          texture->getBindlessId().has_value()) {
        bindlessManager_->removeTexture(texture->getBindlessId().value());
        texture->setBindlessId(std::nullopt);
      }

      if ((imguiRenderer_ != nullptr) && texture->getImguiId().has_value()) {
        imguiRenderer_->unregisterTexture(texture->getImguiId().value());
        texture->setImguiId(std::nullopt);
      }
    }
  }
}

auto TextureStorage::add(const std::shared_ptr<Texture>& texture)
    -> std::uint32_t {
  const std::uint32_t id = vkit::Storage<Texture>::add(texture);

  if (id != kStorageItemInvalidId && texture) {
    const auto lock = std::scoped_lock{this->mutex_};

    const auto graphics_tex = texture->getGraphicsTexture();
    if (graphics_tex) {
      if (bindlessManager_) {
        const auto bindless_id = bindlessManager_->addTexture(graphics_tex);
        texture->setBindlessId(bindless_id);
      }
      if (imguiRenderer_) {
        const auto imgui_id = imguiRenderer_->registerTexture(
            graphics_tex->getImageView(), graphics_tex->getSampler());
        texture->setImguiId(imgui_id);
      }
    }
  }

  return id;
}

void TextureStorage::remove(std::uint32_t id) {
  {
    const auto lock = std::scoped_lock{this->mutex_};

    if (id < this->items_.size() && this->items_[id]) {
      auto texture = this->items_[id];

      if ((bindlessManager_ != nullptr) &&
          texture->getBindlessId().has_value()) {
        bindlessManager_->removeTexture(texture->getBindlessId().value());
        texture->setBindlessId(std::nullopt);
      }

      if ((imguiRenderer_ != nullptr) && texture->getImguiId().has_value()) {
        imguiRenderer_->unregisterTexture(texture->getImguiId().value());
        texture->setImguiId(std::nullopt);
      }
    }
  }

  vkit::Storage<Texture>::remove(id);
}

void TextureStorage::setBindlessManager(
    graphics::BindlessTextureManager* manager) {
  const auto lock = std::scoped_lock{this->mutex_};
  bindlessManager_ = manager;

  if (bindlessManager_) {
    for (auto& tex : this->items_) {
      if (tex && !tex->getBindlessId().has_value() &&
          tex->getGraphicsTexture()) {
        const auto bindless_id =
            bindlessManager_->addTexture(tex->getGraphicsTexture());
        tex->setBindlessId(bindless_id);
      }
    }
  }
}

void TextureStorage::setImguiRenderer(imgui::ImguiRenderer* renderer) {
  const auto lock = std::scoped_lock{this->mutex_};
  imguiRenderer_ = renderer;

  if (imguiRenderer_) {
    for (auto& tex : this->items_) {
      if (tex && !tex->getImguiId().has_value() && tex->getGraphicsTexture()) {
        const auto graphics_tex = tex->getGraphicsTexture();
        const auto imgui_id = imguiRenderer_->registerTexture(
            graphics_tex->getImageView(), graphics_tex->getSampler());
        tex->setImguiId(imgui_id);
      }
    }
  }
}

};  // namespace vkit::texture
